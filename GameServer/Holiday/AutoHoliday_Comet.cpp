#include "AutoHoliday_Comet.h"
#include "../Player/PlayerManager.h"

AutoHoliday_Comet::AutoHoliday_Comet()
{
}

AutoHoliday_Comet::~AutoHoliday_Comet()
{
}

string AutoHoliday_Comet::GetName() const
{
    return "Night Of The Comet";
}

void AutoHoliday_Comet::OnStarted()
{
    string str = "What's that in the sky?? `2A Comet is blazing a trail through the night!`` I t will only be here for 24 hours...";
    GetPlayerManager()->BroadcastMessage(str);

    m_active = true;
}

void AutoHoliday_Comet::OnEnded()
{
    string str = "`2The Comet has passed us by!`` It should return in a month!";
    GetPlayerManager()->BroadcastMessage(str);

    m_active = false;
}
