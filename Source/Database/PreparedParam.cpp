#include "PreparedParam.h"
#include "DatabaseResult.h"

PreparedParam::PreparedParam(const uint8 paramSize)
: m_binds(paramSize), m_buffers(paramSize), m_nulls(paramSize), m_lengths(paramSize)
{
    memset(m_binds.data(), 0, sizeof(MYSQL_BIND) * m_binds.size());
}

PreparedParam::~PreparedParam()
{
}

void PreparedParam::Reset()
{
    if(!m_binds.empty()) {
        memset(m_binds.data(), 0, sizeof(MYSQL_BIND) * m_binds.size());
        
    }
}

void PreparedParam::SetString(const uint8 index, const string& value)
{
    assert(index < m_binds.size());

    m_binds[index].buffer_type = MYSQL_TYPE_STRING;
    
    m_buffers[index].assign(value.begin(), value.end());    
    m_binds[index].buffer = (void*)m_buffers[index].data();
    m_binds[index].buffer_length = (unsigned long)m_buffers[index].size();
    
    m_lengths[index] = m_buffers[index].size();
    m_binds[index].length = &m_lengths[index];
    
    m_nulls[index] = false;
    m_binds[index].is_null = (bool*)&m_nulls[index];
}

void PreparedParam::Load(MYSQL_FIELD* pFields)
{
    if(!pFields) {
        return;
    }

    for(uint32 i = 0; i < m_binds.size(); ++i) {
        m_buffers[i].resize(DatabaseResult::GetSizeOfField(pFields[i]));

        m_binds[i].buffer_type = pFields[i].type;
        m_binds[i].buffer = m_buffers[i].data();
        m_binds[i].buffer_length = m_buffers[i].size();
        m_binds[i].length = &m_lengths[i];
        m_binds[i].is_null = (bool*)&m_nulls[i];
    }
}