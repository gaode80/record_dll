#pragma once

#include <string>
using namespace std;

class SliceIdGenerator
{
public:
	SliceIdGenerator();

	string getNextSliceId();

private:
	string m_slice_id;
	size_t m_ilen;
};

