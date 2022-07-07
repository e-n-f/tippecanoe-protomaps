#ifndef PMTILES_HPP
#define PMTILES_HPP

#include <vector>
#include <fstream>

struct pmtilesv2 {
	std::vector<std::tuple<uint8_t,uint32_t,uint32_t,uint64_t,uint32_t>> entries{};
	uint64_t offset = 0;
	std::ofstream ostream;
};

pmtilesv2 *pmtilesv2_open(const char *filename, char **argv, int force);

void pmtilesv2_write_tile(pmtilesv2 *outfile, int z, int tx, int ty, const char *data, int size);

void pmtilesv2_finalize(pmtilesv2 *outfile);

#endif