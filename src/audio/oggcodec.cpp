/*
 * Copyright (c) 2012-2013, Guillermo A. Amaral B. (gamaral) <g@maral.me>
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

#include "audio/oggcodec.h"

/*!
 * @file
 *
 * @author Guillermo A. Amaral B. (gamaral) <g@maral.me>
 */

#include <cstring>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "core/logger.h"
#include "core/shared.h"

MARSHMALLOW_NAMESPACE_BEGIN
namespace Audio { /****************************************** Audio Namespace */
namespace { /*********************************** Audio::<anonymous> Namespace */

/*
 * The following are callbacks to bridge Ogg Vorbis with out DataIO interface.
 */

struct OVData
{
	Core::SharedDataIO dio;
	long cursor;
};

static size_t
OVRead(void *buffer, size_t byte_size, size_t byte_count, void *data)
{
	if (!data) return(0);

	OVData *l_data = reinterpret_cast<OVData *>(data);

	if (!l_data->dio->seek(l_data->cursor, Core::DIOSet)) {
		MMERROR("Failed to restore DataIO cursor! " << l_data->cursor);
		return(0);
	}

	size_t l_read = l_data->dio->read(buffer, byte_size * byte_count);

	l_data->cursor = l_data->dio->tell();

	return(l_read);
}

static int
OVSeek(void *data, ogg_int64_t offset, int origin)
{
	if (!data) return(-1);

	OVData *l_data = reinterpret_cast<OVData *>(data);

	/* translate origin */
	Core::DIOSeek l_seek;
	switch (origin) {
	case SEEK_SET:
		l_seek = Core::DIOSet;
		break;
	case SEEK_END:
		l_seek = Core::DIOEnd;
		break;
	case SEEK_CUR:
		if (!l_data->dio->seek(l_data->cursor, Core::DIOSet)) {
			MMERROR("Failed to restore DataIO cursor!");
			return(-1);
		}
		l_seek = Core::DIOCurrent;
		break;
	}

	const bool l_result = l_data->dio->seek(offset, l_seek);

	l_data->cursor = l_data->dio->tell();

	return(l_result ? 0 : -1);
}

static long
OVTell(void *data)
{
	if (!data) return(-1);

	OVData *l_data = reinterpret_cast<OVData *>(data);

	return(l_data->cursor);
}

} /********************************************* Audio::<anonymous> Namespace */

struct OggCodec::Private
{
	Private(void)
	    : data(0)
	    , rate(0)
	    , channels(0)
	    , opened(false)
	{}

	inline bool open(const Core::SharedDataIO &_dio);
	inline void close(void);
	inline size_t read(void *buffer, size_t bsize);
	inline void reset(void);

	OVData *data;
	OggVorbis_File handle;

	uint32_t rate;
	uint8_t  channels;
	bool opened;
};

bool
OggCodec::Private::open(const Core::SharedDataIO &_dio)
{
	if (!OggCodec::Validate(_dio)) {
		MMERROR("Tried to open invalid Ogg file.");
		return(false);
	}

	/* init data struct */
	data = new OVData;
	data->dio = _dio;
	data->cursor = 0;

	/* dataio callbacks */
	ov_callbacks l_callbacks;
	l_callbacks.read_func  = OVRead;
	l_callbacks.close_func = 0; /* NO CLOSE */
	l_callbacks.seek_func  = OVSeek;
	l_callbacks.tell_func  = OVTell;

	/* setup callbacks */
	int l_rc;
	if ((l_rc = ov_open_callbacks(data, &handle, 0, 0, l_callbacks))) {
		switch (l_rc) {
		case OV_EREAD:
			MMERROR("A read from media returned an error.");
			break;
		case OV_ENOTVORBIS:
			MMERROR("Bitstream does not contain any Vorbis data.");
			break;
		case OV_EVERSION:
			MMERROR("Vorbis version mismatch.");
			break;
		case OV_EBADHEADER:
			MMERROR("Invalid Vorbis bitstream header.");
			break;
		case OV_EFAULT:
			MMERROR("Internal logic fault; indicates a bug or heap/stack corruption.");
			break;
		}

		MMERROR("Failed to setup Ogg Vorbis <=> DataIO callbacks.");
		delete data, data = 0;
		return(false);
	}

	/* get track info */
	vorbis_info *l_info = ov_info(&handle, -1);
	rate = uint32_t(l_info->rate);
	channels = uint8_t(l_info->channels);

	return(opened = true);
}

void
OggCodec::Private::close(void)
{
	if (!opened) return;

	ov_clear(&handle);
	delete data, data = 0;

	opened = false;
}

size_t
OggCodec::Private::read(void *buffer, size_t bsize)
{
	if (!opened) return(0);

	int l_current_section;

	size_t l_total = 0;

	long l_read = 0;
	do {
		l_read = ov_read(&handle,
		                 reinterpret_cast<char *>(buffer) + l_total,
		                 int(bsize - l_total), 0, 2, 1,
		                 &l_current_section);

		if (l_read > 0) l_total += size_t(l_read);
		else break;
	} while(bsize > l_total);

	/* check for errors */
	if (l_read < 0) {
		MMERROR("Error occurred while reading data!");
		switch (l_read) {
		case OV_HOLE:
			MMDEBUG("OV_HOLE: Data interruption.");
			break;
		case OV_EBADLINK:
			MMDEBUG("OV_EBADLINK: Invalid stream.");
			break;
		case OV_EINVAL:
			MMDEBUG("OV_EINVAL: Invalid header.");
			break;
		}
	}

	return(l_total);
}

void
OggCodec::Private::reset(void)
{
	if (!opened) return;

	if (ov_pcm_seek(&handle, 0))
		MMERROR("Failed to reset codec.");
}

/****************************************************************** OggCodec */

OggCodec::OggCodec(void)
    : m_p(new Private)
{
}

OggCodec::~OggCodec(void)
{
	close();
	delete m_p, m_p = 0;
}

bool
OggCodec::open(const Core::SharedDataIO &dio)
{
	return(m_p->open(dio));
}

void
OggCodec::close(void)
{
	m_p->close();
}

bool
OggCodec::isOpen(void) const
{
	return(m_p->opened);
}

uint32_t
OggCodec::rate(void) const
{
	return(m_p->rate);
}

uint8_t
OggCodec::depth(void) const
{
	return(16);
}

uint8_t
OggCodec::channels(void) const
{
	return(m_p->channels);
}

size_t
OggCodec::read(void *buffer, size_t bsize)
{
	return(m_p->read(buffer, bsize));
}

void
OggCodec::reset(void)
{
	m_p->reset();
}

bool
OggCodec::Validate(const Core::SharedDataIO &dio)
{
	/* sanity check */
	if (!dio->isOpen() && !dio->open(Core::DIOReadOnly)) {
		MMERROR("Failed to open audio stream.");
		return(false);
	}

	/*
	 * Reset location
	 */
	if (!dio->seek(0, Core::DIOSet)) {
		MMDEBUG("Invalid DataIO (failed seek).");
		return(false);
	}

	char l_ident[5] = { 0, 0, 0, 0, 0 };
	
	/* read type */
	if (4 != dio->read(l_ident, 4)) {
		MMDEBUG("Invalid DataIO (short read).");
		return(false);
	}
	
	/* compare chunk type */
	if (strcmp(l_ident, "OggS")) {
		MMDEBUG("Invalid DataIO (not Ogg container)");
		return(false);
	}

	MMDEBUG("Detected Ogg file.");

	if (!dio->seek(0, Core::DIOSet)) {
		MMDEBUG("Invalid DataIO (reset failed).");
		return(false);
	}

	return(true);
}

} /********************************************************** Audio Namespace */
MARSHMALLOW_NAMESPACE_END

