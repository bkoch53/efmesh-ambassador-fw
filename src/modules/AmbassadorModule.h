#pragma once
#include "SinglePortModule.h"

/**
 * A module that DMs new nodes.
 */
class AmbassadorModule : public SinglePortModule
{
  public:
    /** Constructor
     * name is for debugging output
     */
    AmbassadorModule() : SinglePortModule("ambassador", meshtastic_PortNum_REPLY_APP) {}
    void sendAmbassadorMessageToNewNodes(NodeNum n);
    
  protected:
    /** We do all of our processing in the (normally optional)
     * want_replies handling
     */
    virtual void sendPayload(NodeNum dest, bool wantReplies, const char* replyStr);
    virtual ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;
    virtual bool wantPacket(const meshtastic_MeshPacket *p) override;
    void exchangeNodeInfo(NodeNum dest);
};

extern AmbassadorModule *ambassadorModule;

