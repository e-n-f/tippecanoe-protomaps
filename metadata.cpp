#include "metadata.hpp"
#include "mvt.hpp"
#include "text.hpp"

size_t max_tilestats_attributes = 1000;
size_t max_tilestats_sample_values = 1000;
size_t max_tilestats_values = 100;

bool type_and_string::operator<(const type_and_string &o) const {
	if (string < o.string) {
		return true;
	}
	if (string == o.string && type < o.type) {
		return true;
	}
	return false;
}

bool type_and_string::operator!=(const type_and_string &o) const {
	if (type != o.type) {
		return true;
	}
	if (string != o.string) {
		return true;
	}
	return false;
}

void tilestats(std::map<std::string, layermap_entry> const &layermap1, size_t elements, json_writer &state) {
	// Consolidate layers/attributes whose names are truncated
	std::vector<std::map<std::string, layermap_entry>> lmv;
	lmv.push_back(layermap1);
	std::map<std::string, layermap_entry> layermap = merge_layermaps(lmv, true);

	state.json_write_hash();

	state.nospace = true;
	state.json_write_string("layerCount");
	state.json_write_unsigned(layermap.size());

	state.nospace = true;
	state.json_write_string("layers");
	state.json_write_array();

	for (auto layer : layermap) {
		state.nospace = true;
		state.json_write_hash();

		state.nospace = true;
		state.json_write_string("layer");
		state.json_write_string(layer.first);

		state.nospace = true;
		state.json_write_string("count");
		state.json_write_unsigned(layer.second.points + layer.second.lines + layer.second.polygons);

		std::string geomtype = "Polygon";
		if (layer.second.points >= layer.second.lines && layer.second.points >= layer.second.polygons) {
			geomtype = "Point";
		} else if (layer.second.lines >= layer.second.polygons && layer.second.lines >= layer.second.points) {
			geomtype = "LineString";
		}

		state.nospace = true;
		state.json_write_string("geometry");
		state.json_write_string(geomtype);

		size_t attrib_count = layer.second.file_keys.size();
		if (attrib_count > max_tilestats_attributes) {
			attrib_count = max_tilestats_attributes;
		}

		state.nospace = true;
		state.json_write_string("attributeCount");
		state.json_write_unsigned(attrib_count);

		state.nospace = true;
		state.json_write_string("attributes");
		state.nospace = true;
		state.json_write_array();

		size_t attrs = 0;
		for (auto attribute : layer.second.file_keys) {
			if (attrs == elements) {
				break;
			}
			attrs++;

			state.nospace = true;
			state.json_write_hash();

			state.nospace = true;
			state.json_write_string("attribute");
			state.json_write_string(attribute.first);

			size_t val_count = attribute.second.sample_values.size();
			if (val_count > max_tilestats_sample_values) {
				val_count = max_tilestats_sample_values;
			}

			state.nospace = true;
			state.json_write_string("count");
			state.json_write_unsigned(val_count);

			int type = 0;
			for (auto s : attribute.second.sample_values) {
				type |= (1 << s.type);
			}

			std::string type_str;
			// No "null" because null attributes are dropped
			if (type == (1 << mvt_double)) {
				type_str = "number";
			} else if (type == (1 << mvt_bool)) {
				type_str = "boolean";
			} else if (type == (1 << mvt_string)) {
				type_str = "string";
			} else {
				type_str = "mixed";
			}

			state.nospace = true;
			state.json_write_string("type");
			state.json_write_string(type_str);

			state.nospace = true;
			state.json_write_string("values");
			state.json_write_array();

			size_t vals = 0;
			for (auto value : attribute.second.sample_values) {
				if (vals == elements) {
					break;
				}

				state.nospace = true;

				if (value.type == mvt_double || value.type == mvt_bool) {
					vals++;

					state.json_write_stringified(value.string);
				} else {
					std::string trunc = truncate16(value.string, 256);

					if (trunc.size() == value.string.size()) {
						vals++;

						state.json_write_string(value.string);
					}
				}
			}

			state.nospace = true;
			state.json_end_array();

			if ((type & (1 << mvt_double)) != 0) {
				state.nospace = true;
				state.json_write_string("min");
				state.json_write_number(attribute.second.min);

				state.nospace = true;
				state.json_write_string("max");
				state.json_write_number(attribute.second.max);
			}

			state.nospace = true;
			state.json_end_hash();
		}

		state.nospace = true;
		state.json_end_array();
		state.nospace = true;
		state.json_end_hash();
	}

	state.nospace = true;
	state.json_end_array();
	state.nospace = true;
	state.json_end_hash();
}

std::map<std::string, layermap_entry> merge_layermaps(std::vector<std::map<std::string, layermap_entry>> const &maps) {
	return merge_layermaps(maps, false);
}

std::map<std::string, layermap_entry> merge_layermaps(std::vector<std::map<std::string, layermap_entry>> const &maps, bool trunc) {
	std::map<std::string, layermap_entry> out;

	for (size_t i = 0; i < maps.size(); i++) {
		for (auto map = maps[i].begin(); map != maps[i].end(); ++map) {
			if (map->second.points + map->second.lines + map->second.polygons + map->second.retain == 0) {
				continue;
			}

			std::string layername = map->first;
			if (trunc) {
				layername = truncate16(layername, 256);
			}

			if (out.count(layername) == 0) {
				out.insert(std::pair<std::string, layermap_entry>(layername, layermap_entry(out.size())));
				auto out_entry = out.find(layername);
				out_entry->second.minzoom = map->second.minzoom;
				out_entry->second.maxzoom = map->second.maxzoom;
				out_entry->second.description = map->second.description;
			}

			auto out_entry = out.find(layername);
			if (out_entry == out.end()) {
				fprintf(stderr, "Internal error merging layers\n");
				exit(EXIT_FAILURE);
			}

			for (auto fk = map->second.file_keys.begin(); fk != map->second.file_keys.end(); ++fk) {
				std::string attribname = fk->first;
				if (trunc) {
					attribname = truncate16(attribname, 256);
				}

				auto fk2 = out_entry->second.file_keys.find(attribname);

				if (fk2 == out_entry->second.file_keys.end()) {
					out_entry->second.file_keys.insert(std::pair<std::string, type_and_string_stats>(attribname, fk->second));
				} else {
					for (auto val : fk->second.sample_values) {
						auto pt = std::lower_bound(fk2->second.sample_values.begin(), fk2->second.sample_values.end(), val);
						if (pt == fk2->second.sample_values.end() || *pt != val) {  // not found
							fk2->second.sample_values.insert(pt, val);

							if (fk2->second.sample_values.size() > max_tilestats_sample_values) {
								fk2->second.sample_values.pop_back();
							}
						}
					}

					fk2->second.type |= fk->second.type;

					if (fk->second.min < fk2->second.min) {
						fk2->second.min = fk->second.min;
					}
					if (fk->second.max > fk2->second.max) {
						fk2->second.max = fk->second.max;
					}
				}
			}

			if (map->second.minzoom < out_entry->second.minzoom) {
				out_entry->second.minzoom = map->second.minzoom;
			}
			if (map->second.maxzoom > out_entry->second.maxzoom) {
				out_entry->second.maxzoom = map->second.maxzoom;
			}

			out_entry->second.points += map->second.points;
			out_entry->second.lines += map->second.lines;
			out_entry->second.polygons += map->second.polygons;
		}
	}

	return out;
}

void add_to_file_keys(std::map<std::string, type_and_string_stats> &file_keys, std::string const &attrib, type_and_string const &val) {
	if (val.type == mvt_null) {
		return;
	}

	auto fka = file_keys.find(attrib);
	if (fka == file_keys.end()) {
		file_keys.insert(std::pair<std::string, type_and_string_stats>(attrib, type_and_string_stats()));
		fka = file_keys.find(attrib);
	}

	if (fka == file_keys.end()) {
		fprintf(stderr, "Can't happen (tilestats)\n");
		exit(EXIT_FAILURE);
	}

	if (val.type == mvt_double) {
		double d = atof(val.string.c_str());

		if (d < fka->second.min) {
			fka->second.min = d;
		}
		if (d > fka->second.max) {
			fka->second.max = d;
		}
	}

	auto pt = std::lower_bound(fka->second.sample_values.begin(), fka->second.sample_values.end(), val);
	if (pt == fka->second.sample_values.end() || *pt != val) {  // not found
		fka->second.sample_values.insert(pt, val);

		if (fka->second.sample_values.size() > max_tilestats_sample_values) {
			fka->second.sample_values.pop_back();
		}
	}

	fka->second.type |= (1 << val.type);
}
