#ifndef __CSCI_5273_STRINGS_H
#define __CSCI_5273_STRINGS_H

#include <string>

/* Chat Coordinator */
const std::string CMD_COORDINATOR_START		= "Start";
const std::string CMD_COORDINATOR_FIND		= "Find";
const std::string CMD_COORDINATOR_TERMINATE	= "Terminate";

/* Chat Server */
const std::string CMD_SERVER_SUBMIT			= "Submit";
const std::string CMD_SERVER_GET_NEXT		= "GetNext";
const std::string CMD_SERVER_GET_ALL		= "GetAll";
const std::string CMD_SERVER_LEAVE			= "Leave";

/* Chat Client */
const std::string CMD_CLIENT_START			= "Start";
const std::string CMD_CLIENT_JOIN			= "Join";
const std::string CMD_CLIENT_SUBMIT			= "Submit";
const std::string CMD_CLIENT_GET_NEXT		= "GetNext";
const std::string CMD_CLIENT_GET_ALL		= "GetAll";
const std::string CMD_CLIENT_LEAVE			= "Leave";
const std::string CMD_CLIENT_EXIT			= "Exit";

#endif /* __CSCI_5273_STRINGS_H */

