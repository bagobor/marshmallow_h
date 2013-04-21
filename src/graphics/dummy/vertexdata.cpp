/*
 * Copyright (c) 2011-2013, Guillermo A. Amaral B. (gamaral) <g@maral.me>
 * All rights reserved.
 *
 * This file is part of Marshmallow Game Engine.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of the project as a whole.
 */

#include "vertexdata.h"

/*!
 * @file
 *
 * @author Guillermo A. Amaral B. (gamaral) <g@maral.me>
 */

#include <cstring>

MARSHMALLOW_NAMESPACE_BEGIN
namespace Graphics { /************************************ Graphics Namespace */
namespace Dummy { /******************************** Graphics::Dummy Namespace */

VertexData::VertexData(uint16_t c)
#define AXES 2
    : m_data(new float[c * AXES]) // TODO: replace with custom allocator
    , m_count(c)
{
	memset(m_data, 0, m_count * AXES);
}

VertexData::~VertexData(void)
{
	delete[] m_data;
}

bool
VertexData::get(uint16_t i, float &x, float &y) const
{
	const uint16_t l_offset = static_cast<uint16_t>((i % m_count) * AXES);
	x = m_data[l_offset];
	y = m_data[l_offset + 1];
	return(true);
}

bool
VertexData::set(uint16_t i, float x, float y)
{
	const uint16_t l_offset = static_cast<uint16_t>((i % m_count) * AXES);
	m_data[l_offset] = x;
	m_data[l_offset + 1] = y;
	return(true);
}

const Core::Type &
VertexData::Type(void)
{
	static const Core::Type sType("Graphics::Dummy::VertexData");
	return(sType);
}

} /************************************************ Graphics::Dummy Namespace */
} /******************************************************* Graphics Namespace */
MARSHMALLOW_NAMESPACE_END

