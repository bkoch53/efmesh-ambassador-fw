#include "AmbassadorModule.h"
#include "MeshService.h"
#include "configuration.h"
#include "main.h"
#include "NodeDB.h"
#include <stdio.h>
#include <assert.h>
#include "CannedMessageModule.h"
AmbassadorModule *ambassadorModule;

void AmbassadorModule::sendAmbassadorMessageToNewNodes(NodeNum dest)
{
    LOG_WARN("New node 0x%0x sending invite", dest);
    // Create message prefix with node name 
    const meshtastic_NodeInfoLite *node = nodeDB->getMeshNode(dest);
    const char *formatStr = "Hello %s, ";
    char* msgPrefix = new char[strlen(formatStr) + strlen(node->user.short_name)];
    sprintf(msgPrefix, formatStr, node->user.short_name);
    //Get the CannedMessage to send
    uint16_t firstMsgIndex = 1; // index 0 is the [Select Message] menu
    const char *cmStr = cannedMessageModule->getMessageByIndex(firstMsgIndex);
    if (strcmp(cmStr, "[-- Free Text --]") == 0) {
        LOG_INFO("CannedMessage is free text placeholder, incrementing index");
        firstMsgIndex++;
        cmStr = cannedMessageModule->getMessageByIndex(firstMsgIndex);
    }
    if (strcmp(cmStr, "") == 0) {
        LOG_WARN("No CannedMessage configured, not inviting!");
        return;
    }

    char* replyStr = new char[strlen(msgPrefix) + strlen(cmStr) + 1];
    sprintf(replyStr, "%s%s", msgPrefix, cmStr);
    LOG_WARN("Ambassador message to new node %s", replyStr);
    sendPayload(dest, false, replyStr);
}

ProcessMessage AmbassadorModule::handleReceived(const meshtastic_MeshPacket &mp)
{
#if defined(DEBUG_PORT) && !defined(DEBUG_MUTE)
    auto &p = mp.decoded;
    LOG_INFO("Ambassador received text msg from=0x%0x, id=0x%x, msg=%.*s", mp.from, mp.id, p.payload.size, p.payload.bytes);
#endif

    // If the node doesn't have user info yet, initiate an exchange:
    auto sender = mp.from;
    
    const meshtastic_NodeInfoLite *node = nodeDB->getMeshNode(sender);
    bool isNew = (node == NULL);
    LOG_INFO("[Ambassador] Node 0x%0x is new: isNew=%d", sender, isNew);
    
    if (isNew) {
        exchangeNodeInfo(sender);
    } else if (node->has_user == false) {
        LOG_INFO("[Ambassador] Node 0x%0x has no user info %.*s", sender, node->user);
        exchangeNodeInfo(sender);
    } else {
        LOG_INFO("[Ambassador] Node 0x%0x known user pubkey: %.*s sz: %d", sender, node->user.public_key.bytes, node->user.public_key.size);  
    }
    return ProcessMessage::CONTINUE; 
}

void AmbassadorModule::exchangeNodeInfo(NodeNum dest){
    LOG_WARN("[Ambassador] Exchanging node info with node 0x%0x", dest);
    meshtastic_MeshPacket *p = allocDataPacket();
    p->to = dest;
    p->decoded.portnum = meshtastic_PortNum_NODEINFO_APP;
    p->channel = 0; // Primary channel
    p->want_ack = false;
    p->hop_limit = 5;
    p->decoded.want_response = true;
    service->sendToMesh(p);
}

void AmbassadorModule::sendPayload(NodeNum dest, bool wantReplies, const char* msgStr)
{
    LOG_WARN("Sending ambassador reply to node 0x%0x", dest);
    meshtastic_MeshPacket *p = allocDataPacket();
    p->to = dest;
    p->decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
    p->want_ack = true;
    p->hop_limit = 5;
    p->channel = 0; // Primary channel
    p->decoded.want_response = wantReplies;
    p->decoded.payload.size = strlen(msgStr);
    memcpy(p->decoded.payload.bytes, msgStr, p->decoded.payload.size);
    service->sendToMesh(p, RX_SRC_LOCAL, true);
}

bool AmbassadorModule::wantPacket(const meshtastic_MeshPacket *p)
{
    return MeshService::isTextPayload(p);
}