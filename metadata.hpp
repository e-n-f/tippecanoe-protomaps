#ifndef METADATA_HPP
#define METADATA_HPP

#include <string>
#include <vector>
#include <map>
#include <math.h>

#include "write_json.hpp"

extern size_t max_tilestats_attributes;
extern size_t max_tilestats_sample_values;
extern size_t max_tilestats_values;

struct type_and_string {
	int type = 0;
	std::string string = "";

	bool operator<(const type_and_string &o) const;
	bool operator!=(const type_and_string &o) const;
};

struct type_and_string_stats {
	std::vector<type_and_string> sample_values = std::vector<type_and_string>();  // sorted
	double min = INFINITY;
	double max = -INFINITY;
	int type = 0;
};

struct layermap_entry {
	size_t id = 0;
	std::map<std::string, type_and_string_stats> file_keys{};
	int minzoom = 0;
	int maxzoom = 0;
	std::string description = "";

	size_t points = 0;
	size_t lines = 0;
	size_t polygons = 0;
	size_t retain = 0;  // keep for tilestats, even if no features directly here

	layermap_entry(size_t _id) {
		id = _id;
	}
};

std::map<std::string, layermap_entry> merge_layermaps(std::vector<std::map<std::string, layermap_entry> > const &maps);
std::map<std::string, layermap_entry> merge_layermaps(std::vector<std::map<std::string, layermap_entry> > const &maps, bool trunc);

void tilestats(std::map<std::string, layermap_entry> const &layermap1, size_t elements, json_writer &state);

void add_to_file_keys(std::map<std::string, type_and_string_stats> &file_keys, std::string const &layername, type_and_string const &val);

#endif