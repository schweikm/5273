#ifndef __CSCI_5273_STRINGS_H
#define __CSCI_5273_STRINGS_H

/**
 * @file strings.h
 * @author Marc Schweikert
 * @date 26 September 2014
 * @brief String Constants
 */

#include <string>

/** Chat Coordinator - Start */
const std::string CMD_COORDINATOR_START		= "Start";
/** Chat Coordinator - Find */
const std::string CMD_COORDINATOR_FIND		= "Find";
/** Chat Coordinator - Terminate */
const std::string CMD_COORDINATOR_TERMINATE	= "Terminate";

/** Chat Server - Submit */
const std::string CMD_SERVER_SUBMIT			= "Submit";
/** Chat Server - Get Next */
const std::string CMD_SERVER_GET_NEXT		= "GetNext";
/** Chat Server - Get All */
const std::string CMD_SERVER_GET_ALL		= "GetAll";
/** Chat Server - Leave */
const std::string CMD_SERVER_LEAVE			= "Leave";

/** Chat Client - Start */
const std::string CMD_CLIENT_START			= "Start";
/** Chat Client - Join */
const std::string CMD_CLIENT_JOIN			= "Join";
/** Chat Client - Submit */
const std::string CMD_CLIENT_SUBMIT			= "Submit";
/** Chat Client - Get Next */
const std::string CMD_CLIENT_GET_NEXT		= "GetNext";
/** Chat Client - Get All */
const std::string CMD_CLIENT_GET_ALL		= "GetAll";
/** Chat Client - Leave */
const std::string CMD_CLIENT_LEAVE			= "Leave";
/** Chat Client - Exit */
const std::string CMD_CLIENT_EXIT			= "Exit";

#endif /* __CSCI_5273_STRINGS_H */

