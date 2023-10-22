//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream processor control command server.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSProcessorArgs.h"
#include "tstspInputExecutor.h"
#include "tstspProcessorExecutor.h"
#include "tstspOutputExecutor.h"
#include "tsTSPControlCommand.h"
#include "tsThread.h"
#include "tsMutex.h"
#include "tsTCPServer.h"
#include "tsReportWithPrefix.h"

namespace ts {
    namespace tsp {
        //!
        //! Transport stream processor control command server.
        //! This class is internal to the TSDuck library and cannot be called by applications.
        //! @ingroup plugin
        //!
        class ControlServer : public CommandLineHandler, private Thread
        {
            TS_NOBUILD_NOCOPY(ControlServer);
        public:
            //!
            //! Constructor.
            //! @param [in,out] options Command line options for tsp.
            //! @param [in,out] log Log report.
            //! @param [in,out] global_mutex Global mutex to synchronize access to the packet buffer.
            //! @param [in] input Input plugin executor (start of plugin chain).
            //!
            ControlServer(TSProcessorArgs& options, Report& log, Mutex& global_mutex, InputExecutor* input);

            //!
            //! Destructor.
            //!
            virtual ~ControlServer() override;

            //!
            //! Open and start the command listener.
            //! @return True on success, false on error.
            //!
            bool open();

            //!
            //! Stop and close the command listener.
            //!
            void close();

        private:
            volatile bool     _is_open;
            volatile bool     _terminate;
            TSProcessorArgs&  _options;
            ReportWithPrefix  _log;
            TSPControlCommand _reference;
            TCPServer         _server;
            Mutex&            _mutex;
            InputExecutor*    _input;
            OutputExecutor*   _output;
            std::vector<ProcessorExecutor*> _plugins;  // Packet processing plugins

            // Implementation of Thread.
            virtual void main() override;

            // Command handlers.
            CommandStatus executeExit(const UString&, Args&);
            CommandStatus executeSetLog(const UString&, Args&);
            CommandStatus executeList(const UString&, Args&);
            void listOnePlugin(size_t index, UChar type, PluginExecutor* plugin, Report& report);
            CommandStatus executeSuspend(const UString&, Args&);
            CommandStatus executeResume(const UString&, Args&);
            CommandStatus executeSuspendResume(bool state, Args&);
            CommandStatus executeRestart(const UString&, Args&);
        };
    }
}
