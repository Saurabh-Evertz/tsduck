//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// Transport Stream file cleanup utility
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsSignalizationDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsEITProcessor.h"
#include "tsTSFile.h"
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsTS.h"
#include "tsErrCodeReport.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
// Command line options
//----------------------------------------------------------------------------

namespace ts {
    class FileCleanOptions: public Args
    {
        TS_NOBUILD_NOCOPY(FileCleanOptions);
    public:
        FileCleanOptions(int argc, char *argv[]);
        virtual ~FileCleanOptions() override;

        DuckContext           duck {this};  // TSDuck execution context.
        std::vector<fs::path> in_files {};  // Input file names.
        fs::path              out_file {};  // Output file name or directory.
        bool                  out_dir {};   // Output name is a directory.
    };
}

ts::FileCleanOptions::FileCleanOptions(int argc, char *argv[]) :
    Args(u"Cleanup the structure and boundaries of a transport stream file", u"[options] filename ...")
{
    option(u"", 0, FILENAME, 0, UNLIMITED_COUNT);
    help(u"",
         u"MPEG transport stream input files to cleanup. "
         u"All input files must be regular files (no pipe) since the processing is done on two passes. "
         u"If more than one file is specified, the output name shall specify a directory.");

    option(u"output", 'o', FILENAME, 1, 1);
    help(u"output",
         u"Output file or directory. "
         u"This is a mandatory parameter, there is no default. "
         u"If more than one input file is specified, the output name shall specify a directory.");

    analyze(argc, argv);

    getPathValues(in_files, u"");
    getPathValue(out_file, u"output");
    out_dir = fs::is_directory(out_file);

    if (in_files.size() > 1 && !out_dir) {
        error(u"the output name must be a directory when more than one input file is specified");
    }
    exitOnError();
}

ts::FileCleanOptions::~FileCleanOptions()
{
}


//----------------------------------------------------------------------------
// A class to do the file cleanup.
//----------------------------------------------------------------------------

namespace ts {
    class FileCleaner: private SignalizationHandlerInterface
    {
        TS_NOBUILD_NOCOPY(FileCleaner);
    public:
        // Constructor, performing the TS file cleanup.
        FileCleaner(FileCleanOptions& opt, const fs::path& infile_name);

        // Status of the cleanup.
        bool success() const { return _success; }

    private:
        // Context of a PMT PID.
        class PMTContext
        {
            TS_NOBUILD_NOCOPY(PMTContext);
        public:
            // Constructor:
            PMTContext(const DuckContext& duck, PID pmt_pid);

            // Public fields:
            const PID         pmt_pid;
            PMT               pmt;
            CyclingPacketizer pzer;
        };

        // A map of PMT contexts, indexed by PMT PID.
        using PMTContextPtr = std::shared_ptr<PMTContext>;
        using PMTContextMap = std::map<PID,PMTContextPtr>;
        PMTContextPtr getPMTContext(PID pmt_pid, bool create);

        // File cleaner private fields:
        bool               _success = true;
        FileCleanOptions&  _opt;
        TSFile             _in_file {};
        TSFile             _out_file {};
        PAT                _pat {};
        CyclingPacketizer  _pat_pzer {_opt.duck, PID_PAT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        CAT                _cat {};
        CyclingPacketizer  _cat_pzer {_opt.duck, PID_CAT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        SDT                _sdt {};
        CyclingPacketizer  _sdt_pzer {_opt.duck, PID_SDT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        PMTContextMap      _pmts {};

        // Implementation of SignalizationHandlerInterface:
        virtual void handlePAT(const PAT& pat, PID pid) override;
        virtual void handleCAT(const CAT& cat, PID pid) override;
        virtual void handleSDT(const SDT& sdt, PID pid) override;
        virtual void handlePMT(const PMT& pmt, PID pid) override;

        // Close and delete the output file, set error status.
        void errorCleanup();

        // Initialize a packetizer with one table and output the first cycle.
        void initCycle(AbstractLongTable& table, CyclingPacketizer& pzer);

        // Write one packet from a packetizer.
        void writeFromPacketizer(Packetizer& pzer);
    };
}


//----------------------------------------------------------------------------
// File cleaner constructor.
//----------------------------------------------------------------------------

ts::FileCleaner::FileCleaner(FileCleanOptions& opt, const fs::path& infile_name) :
    _opt(opt)
{
    // Mark all tables as invalid. The first occurrence in the input file will initialize them.
    _pat.invalidate();
    _cat.invalidate();
    _sdt.invalidate();

    // Output file name.
    fs::path outfile_name(_opt.out_file);
    if (_opt.out_dir) {
        // Output name is a directory.
        outfile_name /= infile_name.filename();
    }
    _opt.verbose(u"cleaning %s -> %s", {infile_name, outfile_name});

    // Open the input file in rewindable mode.
    if (!_in_file.openRead(infile_name, 0, _opt)) {
        errorCleanup();
        return;
    }

    // Create output file before first pass to avoid spending time on first pass in case of error when creating output.
    if (!_out_file.open(outfile_name, TSFile::WRITE, _opt)) {
        errorCleanup();
        return;
    }

    // First pass: read all packets, process TS structure.
    SignalizationDemux sig(_opt.duck, this, {TID_PAT, TID_CAT, TID_PMT, TID_SDT_ACT});
    TSPacket pkt;
    while (_success && _in_file.readPackets(&pkt, nullptr, 1, _opt) == 1) {
        sig.feedPacket(pkt);
    }

    // Rewind input file to prepare for second pass.
    _success = _success && _in_file.rewind(_opt);

    // Delete output file in case of error in first pass.
    if (!_success) {
        errorCleanup();
        return;
    }

    // Process EIT's in the second pass: keep only EITp/f Actual for known services.
    EITProcessor eit_proc(_opt.duck);
    eit_proc.removeOther();
    eit_proc.removeSchedule();
    for (const auto& it : _pmts) {
        eit_proc.keepService(it.second->pmt.service_id);
    }

    // Start output file. First, issue a full cycle of each PSI/SI.
    initCycle(_pat, _pat_pzer);
    initCycle(_cat, _cat_pzer);
    initCycle(_sdt, _sdt_pzer);
    for (const auto& it : _pmts) {
        initCycle(it.second->pmt, it.second->pzer);
    }

    // In second pass, count input packets per PID.
    std::map<PID,PacketCounter> pkt_count;

    // Second pass: read input file again, write output file.
    while (_success && _in_file.readPackets(&pkt, nullptr, 1, _opt) == 1) {

        // Count input packets per PID.
        const PacketCounter pkt_index = pkt_count[pkt.getPID()]++;

        // Process EIT's. The packet may be nullified (some EIT's are removed).
        eit_proc.processPacket(pkt);

        const PID pid = pkt.getPID();
        const PIDClass pid_class = sig.pidClass(pid);

        if (pid == PID_PAT) {
            writeFromPacketizer(_pat_pzer);
        }
        else if (pid == PID_CAT) {
            writeFromPacketizer(_cat_pzer);
        }
        else if (pid == PID_SDT) {
            writeFromPacketizer(_sdt_pzer);
        }
        else if (pid == PID_EIT || pid_class == PIDClass::ECM || pid_class == PIDClass::EMM) {
            // Write these packets transparently.
            _success = _success && _out_file.writePackets(&pkt, nullptr, 1, _opt);
        }
        else if (pid_class == PIDClass::PSI && Contains(_pmts, pid)) {
            writeFromPacketizer(_pmts[pid]->pzer);
        }
        else if (pid_class == PIDClass::AUDIO || pid_class == PIDClass::SUBTITLES || pid_class == PIDClass::DATA) {
            // Write these packets transparently after the first payload unit start.
            const PacketCounter first_index = sig.pusiFirstIndex(pid);
            if (first_index == INVALID_PACKET_COUNTER || pkt_index >= first_index) {
                _success = _success && _out_file.writePackets(&pkt, nullptr, 1, _opt);
            }
        }
        else if (pid_class == PIDClass::VIDEO) {
            // Write these packets transparently after the first intra-frame (or payload unit start if no IF was found).
            PacketCounter first_index = sig.intraFrameFirstIndex(pid);
            if (first_index == INVALID_PACKET_COUNTER) {
                first_index = sig.pusiFirstIndex(pid);
            }
            if (first_index == INVALID_PACKET_COUNTER || pkt_index >= first_index) {
                _success = _success && _out_file.writePackets(&pkt, nullptr, 1, _opt);
            }
        }
    }

    // Close files.
    _success = _in_file.close(_opt) && _success;
    _success = _out_file.close(_opt) && _success;
}


//----------------------------------------------------------------------------
// Close and delete the output file, set error status.
//----------------------------------------------------------------------------

void ts::FileCleaner::errorCleanup()
{
    if (_in_file.isOpen()) {
        _in_file.close(_opt);
    }
    if (_out_file.isOpen()) {
        const fs::path filename(_out_file.getFileName());
        _out_file.close(_opt);
        fs::remove(filename, &ErrCodeReport(_opt, u"error deleting", filename));
    }
    _success = false;
}


//----------------------------------------------------------------------------
// Invoke for each PAT in the first pass.
//----------------------------------------------------------------------------

void ts::FileCleaner::handlePAT(const PAT& pat, PID pid)
{
    _opt.debug(u"got PAT version %d", {pat.version});
    if (!_pat.isValid()) {
        // First PAT.
        _pat = pat;
        _pat.nit_pid = PID_NULL; // no NIT in output TS
    }
    else {
        // Updated PAT, add new services, check inconsistencies.
        _opt.verbose(u"got PAT update, version %d", {pat.version});
        for (const auto& it : pat.pmts) {
            const auto cur = _pat.pmts.find(it.first);
            if (cur == _pat.pmts.end()) {
                // Add new service in PAT update.
                _opt.verbose(u"added service 0x%X (%<d) from PAT update", {it.first});
                _pat.pmts[it.first] = it.second;
            }
            else if (it.second != cur->second) {
                // Existing service changes PMT PID, not allowed.
                _opt.error(u"service 0x%X (%<d) changed PMT PID from 0x%X (%<d) to 0x%X (%<d) in PAT update", {it.first, cur->second, it.second});
                _success = false;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Invoke for each CAT in the first pass.
//----------------------------------------------------------------------------

void ts::FileCleaner::handleCAT(const CAT& cat, PID pid)
{
    _opt.debug(u"got CAT version %d", {cat.version});
    if (!_cat.isValid()) {
        // First CAT.
        _cat = cat;
    }
    else {
        // Updated CAT, merge descriptors (don't duplicate existing ones).
        _opt.verbose(u"got CAT update, version %d", {cat.version});
        _cat.descs.merge(_opt.duck, cat.descs);
    }
}


//----------------------------------------------------------------------------
// Invoke for each SDT in the first pass.
//----------------------------------------------------------------------------

void ts::FileCleaner::handleSDT(const SDT& sdt, PID pid)
{
    _opt.debug(u"got SDT version %d", {sdt.version});
    if (!_sdt.isValid()) {
        // First SDT.
        _sdt = sdt;
    }
    else {
        // Updated SDT, add new services, merge others.
        _opt.verbose(u"got SDT update, version %d", {sdt.version});
        for (const auto& it : sdt.services) {
            const auto cur = _sdt.services.find(it.first);
            if (cur == _sdt.services.end()) {
                // Add new service in SDT update.
                _opt.verbose(u"added service 0x%X (%<d) from SDT update", {it.first});
                _sdt.services[it.first] = it.second;
            }
            else {
                // Existing service, merge descriptors.
                cur->second.descs.merge(_opt.duck, it.second.descs);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Invoke each time a service is modified in the first pass.
//----------------------------------------------------------------------------

void ts::FileCleaner::handlePMT(const PMT& pmt, PID pid)
{
    _opt.debug(u"got PMT version %d, PID 0x%X (%<d), service id 0x%X (%<d)", {pmt.version, pid, pmt.service_id});

    // Get or create context for this PMT.
    auto ctx = getPMTContext(pid, true);

    if (!ctx->pmt.isValid()) {
        // First PMT on this PID.
        ctx->pmt = pmt;
    }
    else {
        // Updated PMT, add new components, merge others.
        _opt.verbose(u"got PMT update version %d, PID 0x%X (%<d), service id 0x%X (%<d)", {pmt.version, pid, pmt.service_id});
        for (const auto& it : pmt.streams) {
            const auto cur = ctx->pmt.streams.find(it.first);
            if (cur == ctx->pmt.streams.end()) {
                // Add new component in PMT update.
                _opt.verbose(u"added component PID 0x%X (%<d) from PMT update", {it.first});
                ctx->pmt.streams[it.first] = it.second;
            }
            else {
                // Existing component, merge descriptors.
                cur->second.descs.merge(_opt.duck, it.second.descs);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Context of a PMT PID.
//----------------------------------------------------------------------------

ts::FileCleaner::PMTContext::PMTContext(const DuckContext& duck, PID pid) :
    pmt_pid(pid),
    pmt(),
    pzer(duck, pmt_pid, CyclingPacketizer::StuffingPolicy::ALWAYS)
{
    pmt.invalidate();
}

ts::FileCleaner::PMTContextPtr ts::FileCleaner::getPMTContext(PID pmt_pid, bool create)
{
    auto it = _pmts.find(pmt_pid);
    if (it != _pmts.end()) {
        return it->second;
    }
    else if (create) {
        return _pmts[pmt_pid] = PMTContextPtr(new PMTContext(_opt.duck, pmt_pid));
    }
    else {
        return PMTContextPtr();
    }
}


//----------------------------------------------------------------------------
// Initialize a packetizer with one table and output the first cycle.
//----------------------------------------------------------------------------

void ts::FileCleaner::initCycle(AbstractLongTable& table, CyclingPacketizer& pzer)
{
    if (table.isValid()) {
        table.version = 0;
        table.is_current = true;
        pzer.addTable(_opt.duck, table);
        do {
            writeFromPacketizer(pzer);
        } while (_success && !pzer.atCycleBoundary());
    }
}


//----------------------------------------------------------------------------
// Write one packet from a packetizer.
//----------------------------------------------------------------------------

void ts::FileCleaner::writeFromPacketizer(Packetizer& pzer)
{
    TSPacket pkt;
    if (_success && pzer.getNextPacket(pkt) && !_out_file.writePackets(&pkt, nullptr, 1, _opt)) {
        _success = false;
    }
}


//----------------------------------------------------------------------------
// Program entry point.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    ts::FileCleanOptions opt(argc, argv);
    bool success = true;

    for (const auto& file : opt.in_files) {
        ts::FileCleaner fclean(opt, file);
        success = success && fclean.success();
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
