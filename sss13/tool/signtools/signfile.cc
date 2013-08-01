/*
 * \brief  Sign file
 * \author Ivan Loskutov <ivan.loskutov@ksyslabs.org>
 * \date   2013-07-13
 */

/*
 * Copyright (C) 2013 Ksys Labs LLC
 * Copyright (C) 2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>

extern "C" {
#include <micro-ecc/ecc.h>
}

#include <sha2/sha2.h>

struct RandomReadError {};

void usage(const std::string &prog)
{
	std::cerr << "Usage:\n" << prog << " file private_key" << std::endl;
}

void get_random(void *dest, unsigned size)
{
	std::fstream rand("/dev/random", std::ios::in | std::ios::binary);
	if (!rand.is_open()) {
		std::cerr << "Failed to open /dev/random" << std::endl;
		throw RandomReadError();
	}
	rand.read(reinterpret_cast<char*>(dest), size);
	if (rand.gcount() != size) {
		rand.close();
		std::cerr << "Failed to get random bytes." << std::endl;
		throw RandomReadError();
	}
	rand.close();
}

int main(int argc, char** argv)
{
	int rc = 0;
	
	if (argc != 3) {
		usage(argv[0]);
		return 1;
	}

	std::ifstream file(argv[1], std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "Failed to open file " << argv[1] << std::endl;
		return 1;
	}
	file.seekg(0, file.end);
	long size = file.tellg();
	file.seekg(0, file.beg);
	
	// TODO: calculate dataspace size
	long data_size = ???;
	
	unsigned char *data = new unsigned char[data_size];
	memset(data, 0, data_size);
	
	file.read((char*)data, size);
	file.close();
	
	// TODO: calculate SHA256 hash
	unsigned char hash[SHA256_DIGEST_LENGTH];
	
	delete [] data;
	
	// read private key
	std::ifstream key(argv[2], std::ios::binary);
	if (!key.is_open()) {
		std::cerr << "can't open key " << argv[2] << std::endl;
		return 1;
	}
	uint32_t pkey[NUM_ECC_DIGITS];
	key.read( reinterpret_cast<char*>(pkey), sizeof(uint32_t) * NUM_ECC_DIGITS);
	key.close();
	
	uint32_t sign_r[NUM_ECC_DIGITS];
	uint32_t sign_s[NUM_ECC_DIGITS];
	uint32_t random[NUM_ECC_DIGITS];
	
	try {
		get_random(random, sizeof(random[0]) * NUM_ECC_DIGITS);
		
		// sign hash
		if( !ecdsa_sign(sign_r, sign_s, pkey, random, reinterpret_cast<uint32_t*>(hash)) ) {
			std::cerr << "ecdsa_sign() failed" << std::endl;
			return 1;
		}
		
		// print signature to stdout 
		for (int i(0); i != NUM_ECC_DIGITS * sizeof(sign_r[0]); ++i)
			std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned int>(reinterpret_cast<unsigned char*>(sign_r)[i]);
		for (int i(0); i != NUM_ECC_DIGITS * sizeof(sign_s[0]); ++i)
			std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned int>(reinterpret_cast<unsigned char*>(sign_s)[i]);
		std::cout << std::endl;
	}
	catch(...) {
		std::cerr << "error" << std::endl;
		rc = 1;
	}
	
	return rc;
}
