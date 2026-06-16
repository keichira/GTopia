#include "CrystalBlockDialog.h"
#include "Item/ItemInfoManager.h"
#include "../../World/WorldManager.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"
#include "../../Item/HarmonicCrystal.h"
#include "Math/Math.h"

void CrystalBlockDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if(!pPlayer || !pTile || !pItem)
        return;

    if(pItem->type != ITEM_TYPE_CRYSTAL)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    TileExtra_Crystal* pTileExtra = pTile->GetExtra<TileExtra_Crystal>();
    if(!pTileExtra)
        return;

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon("`wCrystal Cluster", pItem->id, true);

    string clusterCrystals = "This is a cluster of ";

    for(uint32 i = 0; i < pTileExtra->crystals.size();)
    {
        char c = pTileExtra->crystals[i];
        int32 cnt = 0;
    
        while(i < pTileExtra->crystals.size() && pTileExtra->crystals[i] == c)
            cnt++, i++;
    
        clusterCrystals +=
            (cnt == 1 ? "a few " :
            cnt == 2 ? "several " :
            cnt == 3 ? "lots of " :
            cnt == 4 ? "tons of " : "nothing but ");
    
        clusterCrystals +=
            (c == '1' ? "red" :
            c == '2' ? "green" :
            c == '3' ? "blue" :
            c == '4' ? "white" : "black");
    
        if(i < pTileExtra->crystals.size())
            clusterCrystals += ", ";
    }

    db.AddTextBox(clusterCrystals + "crystals.");

    if(pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
    {
        int16 totalChi[4] = { 0 };
        pWorld->CalcHarmonicCrystal(pTile, totalChi);

        int32 accur = gHarmonicCrystal.GetChiAccuracy(pTile, pWorld);
        bool ready = true;
        int16 diff[4] = { 0 };

        for(int32 i = 0; i < 4; ++i)
        {
            diff[i] = pTileExtra->chi[i] - totalChi[i];
        
            if(diff[i] < -accur || diff[i] > accur)
                ready = false;
        }

        string elements[4] = {
            "Earth",
            "Fire",
            "Air",
            "Water"
        };

        string elementInfo;

        if(!ready)
        {
            elementInfo = "The crystals are out of alignment with the elements. They need to be surrounded by ";
            bool first = true;

            for(int32 i = 0; i < 4; ++i)
            {
                string state;
            
                if(diff[i] < -accur)
                {
                    if(diff[i] < -accur * 5) state = "vastly less";
                    else if(diff[i] < -accur * 3) state = "far less";
                    else if(diff[i] < -accur * 2) state = "less";
                    else state = "slightly less";
                }
                else if(diff[i] > accur)
                {
                    if(diff[i] > accur * 5) state = "vastly more";
                    else if(diff[i] > accur * 3) state = "far more";
                    else if(diff[i] > accur * 2) state = "more";
                    else state = "slightly more";
                }
                else
                    continue;
            
                if(!first)
                    elementInfo += ", ";
            
                first = false;
                elementInfo += state + " " + elements[i];
            }

            elementInfo += " to seek a higher form.";
        }
        else
        {
            elementInfo = "The crystals are in perfect harmonic resonance with the elements. A single tap should reveal their true essence.";
        }

        db.AddTextBox(elementInfo);
    }

    db.EndDialog("crystal_edit", "Cool, man", "");
    pPlayer->SendOnDialogRequest(db.Get());
}