#include "Context.h"
#include "Utils/ResourceManager.h"
#include "IO/Log.h"

Context::Context()
{
}

Context::~Context()
{
}

void Context::Init()
{
    m_pGameConfig = new GameConfig();
}

void Context::Kill()
{
    ContextBase::Kill();

    SAFE_DELETE(m_pGameConfig);
}

void Context::LoadPreResources()
{
    ResourceManager* pResMgr = GetResourceManager();

#ifdef _DEBUG
    if(!pResMgr->LoadFont(FONT_TYPE_CENTURY_GOTHIC_BOLD, "centurygothic_bold.ttf")) {
        LOGGER_LOG_ERROR("Failed to load font %d", FONT_TYPE_CENTURY_GOTHIC_BOLD);
    }
#endif
}

Context* GetContext() { return Context::GetInstance(); }