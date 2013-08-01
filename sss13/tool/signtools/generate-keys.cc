/*
 * \brief  Generate private/public keys for signing
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

extern "C" {
	#include <micro-ecc/ecc.h>
}

struct RandomRead {};
struct PrivateWrite {};
struct PublicWrite {};

void vli_print(uint32_t *vli)
{
	for(int i(0); i != NUM_ECC_DIGITS - 1; ++i)
		std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << vli[i] << ", ";
	std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << vli[NUM_ECC_DIGITS - 1];
}

void private_write(uint32_t *vli, const std::string &dir)
{
	std::ofstream key((dir + "private.key").c_str(), std::ios::binary | std::ios::trunc);
	if (!key.is_open()) {
		std::cerr << "Failed to create private.key" << std::endl;
		throw PrivateWrite();
	}
	key.write( reinterpret_cast<char*>(vli), sizeof(vli[0]) * NUM_ECC_DIGITS);
	key.close();
}

void get_random(void *dest, unsigned size)
{
	std::fstream rand("/dev/random", std::ios::in | std::ios::binary);
	if (!rand.is_open()) {
		std::cerr << "Failed to open /dev/random" << std::endl;
		throw RandomRead();
	}
	rand.read(reinterpret_cast<char*>(dest), size);
	if (rand.gcount() != size)
	{
		rand.close();
		std::cerr << "Failed to get random bytes." << std::endl;
		throw RandomRead();
	}
	rand.close();
}

int main(int argc, char **argv)
{
	int rc = 0;
	
	if(argc != 2)
	{
		std::cerr << "Usage:\n" << argv[0] << " key_dir" << std::endl;
		return 1;
	}
	
	std::string kdir(argv[1]);

	uint32_t random[NUM_ECC_DIGITS];
	uint32_t k_private[NUM_ECC_DIGITS];
	EccPoint k_public;

	try {
		get_random((char *)random, NUM_ECC_DIGITS * sizeof(uint32_t));
		ecc_make_key(&k_public, k_private, random);

		std::cout << "#pragma once" << std::endl << std::endl;
		std::cout << "EccPoint k_public = {" << std::endl;
		std::cout << "\t{ ";
		vli_print(k_public.x);
		std::cout << " }," << std::endl;
		std::cout << "\t{ ";
		vli_print(k_public.y);
		std::cout << " }\n};" << std::endl << std::endl;

		private_write(k_private, kdir);
	}
	catch(...) {
		std::cerr << "error" << std::endl;
		rc = 1;
	}

	return rc;
}
