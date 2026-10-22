#ifndef SPIFFSCHECKSUM_HPP
#define SPIFFSCHECKSUM_HPP

#include <cstddef>

struct {
	const char* name;
	size_t size;
} static const SPIFFSFiles[] = {
	{ "/spiffs_image/intro.sz.hs", 49589 },
	{ "/spiffs_image/pairing.sz.hs", 69425 },
	{ "/spiffs_image/t_1.sz.hs", 77049 },
	{ "/spiffs_image/t_2.sz.hs", 74390 },
	{ "/spiffs_image/t_3.sz.hs", 75003 },
	{ "/spiffs_image/t_4.sz.hs", 70782 },
};

#endif //SPIFFSCHECKSUM_HPP
