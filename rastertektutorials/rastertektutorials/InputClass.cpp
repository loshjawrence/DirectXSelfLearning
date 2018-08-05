#include "inputclass.h"

InputClass::InputClass()
{
}

InputClass::InputClass(const InputClass& copyFrom)
{
}

InputClass::~InputClass()
{
}

void InputClass::Initialize()
{
	for (int i = 0; i < 256; ++i)
		m_keys[i] = false;
}

void InputClass::KeyDown(unsigned int input)
{
	m_keys[input] = true;
}

void InputClass::KeyUp(unsigned int input)
{
	m_keys[input] = false;
}

bool InputClass::IsKeyDown(unsigned int input)
{
	return m_keys[input];
}
