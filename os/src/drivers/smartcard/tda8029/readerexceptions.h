/*
 * \brief  Exceptions for TDA8029 driver
 * \author Ivan Loskutov <ivan.loskutov@ksyslabs.org>
 * \date   2012-10-13
 */

/*
 * Copyright (C) 2012 Ksys Labs LLC
 * Copyright (C) 2011-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _READER_EXCEPTIONS_H_
#define _READER_EXCEPTIONS_H_

class ReaderException
{
	const char *_msg;
public:
	ReaderException(const char *msg): _msg(msg) {}
	virtual ~ReaderException() {}
	const char* what() const { return _msg; }
};

class DataTooLong: public ReaderException {
public:
	DataTooLong(const char *msg = "Data too long"): ReaderException(msg) {}
};

class UnknownHeaderSignature: public ReaderException {
public:
	UnknownHeaderSignature(const char *msg = "Unknown header signature"): ReaderException(msg) {}
};

class Timeout: public ReaderException {
public:
	Timeout(const char *msg = "Timeout"): ReaderException(msg) {}
};

class BadLRC: public ReaderException {
public:
	BadLRC(const char *msg = "Bad LRC in response"): ReaderException(msg) {}
};

class AlparNac: public ReaderException {
public:
	AlparNac(const char *msg = "NAC in response"): ReaderException(msg) {}
};

#endif // _READER_EXCEPTIONS_H_
