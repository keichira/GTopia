#include "DialogBuilder.h"

DialogBuilder::DialogBuilder()
{
}

DialogBuilder::~DialogBuilder()
{
}

DialogBuilder* DialogBuilder::AddTextBox(const string& str, bool center)
{
    m_str += "add_textbox|" + str + "|";

    if(center) {
        m_str += "center|\n";
    }
    else {
        m_str += "\n";
    }

    return this;
}

DialogBuilder* DialogBuilder::AddLabelWithIcon(const string& str, uint16 itemID, bool big, bool center)
{
    m_str += "add_label_with_icon|";

    if(big) m_str += "big|";
    else m_str += "small|";

    m_str += str + "|";

    if(center) m_str += "center|";
    else m_str += "left|";

    m_str += ToString(itemID) + "|\n";
    return this;
}

DialogBuilder* DialogBuilder::AddButton(const string& buttonID, const string& text, const string& flags)
{
    m_str += "add_button|" + buttonID  + "|" + text + "|" + flags + "|0|0|\n"; // urlPrompt?
    return this;
}

DialogBuilder* DialogBuilder::AddTextInput(const string& buttonID, const string& text, const string& placeholder, uint32 inputMaxLength)
{
    m_str += "add_text_input|" + buttonID + "|" + text + "|" + placeholder + "|" + ToString(inputMaxLength) + "|\n";
    return this;
}

DialogBuilder* DialogBuilder::AddTextInputPassword(const string& buttonID, const string& text, const string& placeholder, uint32 inputMaxLength)
{
    m_str += "add_text_input_password|" + buttonID + "|" + text + "|" + placeholder + "|" + ToString(inputMaxLength) + "|\n";
    return this;
}

DialogBuilder* DialogBuilder::EndDialog(const string& dialogID, const string& acceptText, const string& cancelText)
{
    m_str += "end_dialog|" + dialogID + "|" + cancelText + "|" + acceptText + "|\n";
    return this;
}

DialogBuilder* DialogBuilder::SetDefaultColor(char colorID)
{
    m_str += "set_default_color|`";
    m_str += colorID;
    m_str += "|\n";
    return this;
}

DialogBuilder* DialogBuilder::AddSpacer(bool big)
{
    m_str += "add_spacer|";

    if(big) m_str += "big|\n";
    else m_str += "small|\n";

    return this;
}

DialogBuilder* DialogBuilder::AddCheckBox(const string& boxID, const string& text, bool active)
{
    m_str += "add_checkbox|" + boxID + "|" + text + "|";
    m_str += active ? "1|\n" : "0|\n";

    return this;
}

DialogBuilder* DialogBuilder::AddPlayerInfo(const string& label, uint32 level, uint32 currentXP, uint32 XPToLevelUP)
{
    m_str += "add_player_info|" + label + "|" + ToString(level) + "|" + ToString(currentXP) + "|" + ToString(XPToLevelUP) + "|\n";
    return this;
}

DialogBuilder* DialogBuilder::AddCustomButton(const string& buttonID, const string& data)
{
    m_str += "add_custom_button|" + buttonID + "|" + data + "|\n";
    return this;
}

DialogBuilder* DialogBuilder::AddButton(const string& buttonID, const string& name, const string& texturePath,
                                        const string& description, uint8 posX, uint8 posY, int32 gemCost, int32 videoCreditCost,
                                        const string& overlayText, const string& ovelayTexture, int32 overlayPosX, int32 overlayPosY, 
                                        const string& popupTexture, int32 popupPosX, int32 popupPosY, bool enabled,
                                        const string& disabledTexture, int32 disabledPosX, int32 disabledPosY)
{
    m_str += "add_button|" + buttonID + "|" + name + "|" + texturePath + "|" + description + "|" + ToString(posX) + "|" + ToString(posY) + "|";
    m_str += ToString(gemCost) + "|" + ToString(videoCreditCost) + "|" + overlayText + "|" + ovelayTexture + "|" + ToString(overlayPosX) + "|" + ToString(overlayPosY) + "|";
    m_str += popupTexture + "|" + ToString(popupPosX) + "|" + ToString(popupPosY) + "|" + "||"; 
    m_str += enabled ? "1|" : "0|";
    m_str += "||||" + disabledTexture + "|" + ToString(disabledPosX) + "|" + ToString(disabledPosY) + "|\n";

    return this;
}

DialogBuilder* DialogBuilder::SetDescriptionText(const string& text)
{
    m_str += "set_description_text|" + text + "|\n";
    return this;
}

DialogBuilder* DialogBuilder::AddTabButton(const string& buttonID, const string& name, const string& texturePath, const string& description, bool active, uint8 textureY)
{
    AddButton(buttonID, name, texturePath, description, active ? 1 : 0, textureY);
    
    usize pos = m_str.rfind("add_button");
    if(pos != string::npos)
    {
        m_str.replace(pos, 10, "add_tab_button");
    }

    return this;
}

DialogBuilder* DialogBuilder::AddLabel(const string& label, bool big)
{
    m_str += "add_label|";

    if(big) m_str += "big|";
    else m_str += "small|";

    m_str += label + "|\n";

    return this;
}

DialogBuilder* DialogBuilder::AddPlayerPicker(const string& id, const string& label)
{
    m_str += "add_player_picker|" + id + "|" + label + "|\n";
    return this;
}

DialogBuilder* DialogBuilder::AddSmallText(const string& text, bool center)
{
    m_str += "add_small_text|" + text;

    if(center) m_str += "|center|\n";
    else m_str += "|left|\n";

    return this;
}

DialogBuilder* DialogBuilder::AddAchieveButton(const string& name, const string& desc, uint8 id)
{
    m_str += "add_achieve_button|" + name + "|" + desc + "|left|" + ToString(id) + "|\n";
    return this;
}

DialogBuilder* DialogBuilder::AddItemPicker(const string& id, const string& buttonText, const string& label)
{
    m_str += "add_item_picker|" + id + "|" + buttonText + "|" + label + "|\n";
    return this;
}

DialogBuilder* DialogBuilder::EmbedData(const string& name, const string& value)
{
    m_str += "embed_data|" + name + "|" + value + "\n";
    return this;
}
