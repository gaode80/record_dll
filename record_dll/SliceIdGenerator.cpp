#include "SliceIdGenerator.h"

SliceIdGenerator::SliceIdGenerator()
{
	m_slice_id = "aaaaaaaaaa";
	m_ilen = m_slice_id.length();
}

string  SliceIdGenerator::getNextSliceId()
{
	for (int i = 0, j = m_ilen - 1; i < m_ilen && j >= 0; i++)
	{
		if (m_slice_id[j] != 'z')
		{
			m_slice_id[j]++;
			break;
		}
		else
		{
			m_slice_id[j] = 'a';
			j--;
			continue;
		}
	}

	return m_slice_id;
}