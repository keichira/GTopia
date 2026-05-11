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