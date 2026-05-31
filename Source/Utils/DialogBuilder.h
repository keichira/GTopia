#pragma once

#include "../Precompiled.h"
#include "StringUtils.h"

class DialogBuilder {
public:
    DialogBuilder();
    ~DialogBuilder();

public:
    void Reset() { m_str.clear(); }
    const string& Get() const { return m_str; }
    
    DialogBuilder* AddTextBox(const string& str, bool center = false);
    DialogBuilder* AddLabelWithIcon(const string& str, uint16 itemID, bool big = false, bool center = false);
    DialogBuilder* AddButton(const string& buttonID, const string& text, const string& flags = "NOFLAGS");
    DialogBuilder* AddTextInput(const string& buttonID, const string& text, const string& placeholder, uint32 inputMaxLength = 10);
    DialogBuilder* AddTextInputPassword(const string&buttonID, const string& text, const string& placeholder, uint32 inputMaxLength = 10);
    DialogBuilder* EndDialog(const string& dialogID, const string& acceptText, const string& cancelText);
    DialogBuilder* SetDefaultColor(char colorID);
    DialogBuilder* AddSpacer(bool big = false);
    DialogBuilder* AddCheckBox(const string& boxID, const string& text, bool active);
    DialogBuilder* AddPlayerInfo(const string& label, uint32 level, uint32 currentXP, uint32 XPToLevelUP);
    DialogBuilder* AddCustomButton(const string& buttonID, const string& data);
    DialogBuilder* AddButton(const string& buttonID, const string& name, const string& texturePath,
                            const string& description, uint8 posX, uint8 posY, int32 gemCost = 0, int32 videoCreditCost = 0,
                            const string& overlayText = "", const string& ovelayTexture = "", int32 overlayPosX = -1, int32 overlayPosY = -1, 
                            const string& popupTexture = "", int32 popupPosX = -1, int32 popupPosY = -1, bool enabled = true,
                            const string& disabledTexture = "", int32 disabledPosX = 0, int32 disabledPosY = 0);
    DialogBuilder* SetDescriptionText(const string& text);
    DialogBuilder* AddTabButton(const string& buttonID, const string& name, const string& texturePath, 
                               const string& description, bool active, uint8 textureY);
    DialogBuilder* AddLabel(const string& label, bool big = false);
    DialogBuilder* AddPlayerPicker(const string& id, const string& label);
    DialogBuilder* AddSmallText(const string& text, bool center = false);
    DialogBuilder* AddAchieveButton(const string& name, const string& desc, uint8 id);
    DialogBuilder* AddItemPicker(const string& id, const string& buttonText, const string& label);

    DialogBuilder* EmbedData(const string& name, const string& value);

    template<typename T>
    DialogBuilder* EmbedData(const string& name, const T& value)
    {
        EmbedData(name, ToString(value));
        return this;
    }

private:
    string m_str;
};