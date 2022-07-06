#include <stdio.h>
#include "pmtiles.hpp"

pmtilesv2 *pmtilesv2_open(const char *filename, char **argv, int forcetable) {
	pmtilesv2 *outfile = new pmtilesv2;
	return outfile;
}

void pmtilesv2_write_tile(pmtilesv2 *outfile, int z, int tx, int ty, const char *data, int size) {
	outfile->entries.emplace_back(z,tx,ty,outfile->offset,size);
	fprintf(stderr,"%d %d %d\n", z, tx, ty);
	outfile->offset+=size;
}

void pmtilesv2_finalize(pmtilesv2 *outfile, const char *pgm) {
	fprintf(stderr, "offset: %llu\n", outfile->offset);
	fprintf(stderr, "entries: %lu\n", outfile->entries.size());
	delete outfile;
};