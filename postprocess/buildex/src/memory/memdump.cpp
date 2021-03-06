#include <sys\stat.h>
#include <iostream>
#include <string>
#include <stdlib.h>

#include "utility/defines.h"
#include "utilities.h"
#include "imageinfo.h"
#include "memory/memdump.h"

/*
* following are mem layout dependant functions which convert concrete mem layout into a abstracted D-dimensional strucuture
* function naming convention -
C,R - column or row major layout
N,E - embedded or not embedded colors
1,2,3... - number of dimensions searching for
*/

/* basic photoshop image layout - invert, blur etc. */
mem_regions_t * locate_image_CN2(char * values, uint32_t size, uint64_t * start_line, uint64_t * end_line, image_t * image){

	/* we will be searching for the first line of the image */

	bool image_found = false;
	int i;
	uint32_t count = 0;

	for (i = *start_line; i < size; i++){

		print_progress(&count, 100000);


		bool success = true;
		for (int j = 0; j < image->width; j++){
			if ((values[i + j] & 0xff) != image->image_array[j]){ /* this may need to be changed */
				success = false;
				break;
			}
		}

		if (success){ /* verify the region actually has the image */
			vector<uint32_t> start_points;
			int start = 0;
			int last = 0;
			/* get the starting points of each image line */
			for (int j = i; j < size; j++){
				bool found = true;
				for (int k = 0; k < image->width; k++){
					if ((values[j + k] & 0xff) != image->image_array[last + k]){
						found = false;
						break;
					}
				}
				if (found){
					start = j;
					last += image->width;
					j += image->width - 1;
					start_points.push_back(start);
					if (last == image->width * image->height) {
						*end_line = j;
						break; 
					}
				}
			}
			/* if we covered the entire image */
			if (last == image->width * image->height){

				/* check whether the gaps are uniform */
				vector<uint32_t> gaps;
				for (int j = 1; j < start_points.size(); j++){
					gaps.push_back(start_points[j] - start_points[j - 1]);
				}

				if (gaps.size() == 0){
					mem_regions_t * region = new mem_regions_t();

					region->bytes_per_pixel = 1;
					region->dimensions = 2;

					region->strides[0] = 1;
					region->strides[1] = image->width;

					region->padding_filled = 1;
					region->padding[0] = 0;

					region->extents[0] = image->width;
					region->extents[1] = image->height;

					/*region start and the end */
					region->start = start_points[0];
					region->end = start_points[start_points.size() - 1] + region->strides[1];

					return region;
				}
				else{

					uint32_t found = true;
					uint32_t comp_val = gaps[0];
					for (int j = 1; j < gaps.size(); j++){
						if (comp_val != gaps[j]){
							found = false; break;
						}
					}
					if (found){
						mem_regions_t * region = new mem_regions_t();

						region->bytes_per_pixel = 1;
						region->dimensions = 2;

						region->strides[0] = 1;
						region->strides[1] = gaps[0];

						region->padding_filled = 1;
						region->padding[0] = gaps[0] - image->width;

						region->extents[0] = image->width;
						region->extents[1] = image->height;

						/* region start and the end */
						region->start = start_points[0];
						region->end = start_points[start_points.size() - 1] + region->strides[1];

						return region;
					}
				}
			}
		}
	}

	return NULL;

}

mem_regions_t * locate_image_CN2_backward_write(char * values, uint32_t size, uint64_t * start, uint64_t * end, image_t * image, uint32_t bound){

	bool image_found = false;
	int i;
	uint32_t count = 0;
	uint32_t image_size = image->width * image->height;


	for (i = *start; i >= 3 * image_size; i--){

		print_progress(&count, 100000);


		bool success = true;
		for (int j = image->width - 1 - bound; j >= bound; j--){
			int val = 3 * (image->width - 1 - bound - j);
			if ((values[i - val] & 0xff) != image->image_array[(0 * image->width * image->height) + j + bound * image->width]){
				success = false;
				break;
			}
			if ((values[i - (val + 1)] & 0xff) != image->image_array[ (1 * image->width * image->height) + j + bound * image->width]){ /* this may need to be changed */
				success = false;
				break;
			}
			if ((values[i - (val + 2)] & 0xff) != image->image_array[ (2 * image->width * image->height) + j + bound * image->width]){ /* this may need to be changed */
				success = false;
				break;
			}

		}


		if (success){ /* verify the region actually has the image */
			vector<uint32_t> start_points;
			int start_local = 0;
			int last = 0;
			/* get the starting points of each image line */
			for (int j = i; j >= image_size; j--){

				bool found = true;
				for (int k = image->width - 1 - bound; k >= bound; k--){
					int val = 3 * (image->width - 1 - bound - k);
					if ((values[j - (val)] & 0xff) != (uint8_t)(image->image_array[(0 * image->width * image->height) + last + k + bound * image->width])){
						found = false;
						break;
					}
					if ((values[j - (val + 1)] & 0xff) != (uint8_t)image->image_array[(1 * image->width * image->height) + last + k + bound * image->width]){
						found = false;
						break;
					}
					if ((values[j - (val + 2)] & 0xff) != (uint8_t)image->image_array[(2 * image->width * image->height) + last + k + bound * image->width]){
						found = false;
						break;
					}
				}



				if (found){
					start_local = j;
					last += image->width;
					j -= 3 * (image->width - 2 * bound) - 1; /* because of j--*/
					start_points.push_back(start_local);
					if (last == image->width * (image->height - 2 * bound) ) { break; }
				}

			}
			/* if we covered the entire image */
			if (last == image->width * (image->height - 2 * bound) ){

				/* check whether the gaps are uniform */
				vector<uint32_t> gaps;
				for (int j = 0; j < start_points.size() - 1; j++){
					gaps.push_back(start_points[j] - start_points[j + 1]);
				}

				uint32_t found = true;
				uint32_t comp_val = gaps[0];
				for (int j = 1; j < gaps.size(); j++){
					if (comp_val != gaps[j]){
						found = false; break;
					}
				}
				if (found){
					mem_regions_t * region = new mem_regions_t();

					region->bytes_per_pixel = 1;
					region->dimensions = 3;

					region->strides[0] = 1;
					region->strides[1] = 3;
					region->strides[2] = gaps[0];

					region->padding_filled = 1;
					if (gaps[0] != (image->width - 2 * bound) * 3){
						region->padding[0] = gaps[0] - (image->width - 2 * bound) * 3;
					}
					else region->padding[0] = 0;

					region->extents[0] = 3;
					region->extents[1] = image->width - 2 * bound;
					region->extents[2] = image->height - 2 * bound;

					/* region start and the end */
					region->start = start_points[0];
					region->end = start_points[start_points.size() - 1] - region->strides[2];
					*end = region->end;
					return region;
				}


			}
		}
	}

	return NULL;
}

mem_regions_t * locate_image_CN2_backward(char * values, uint32_t size, uint64_t * start, uint64_t * end, image_t * image){

	bool image_found = false;
	int i;
	uint32_t count = 0;
	uint32_t image_size = image->width * image->height;


	for (i = *start; i >= 3 * image_size; i--){

		print_progress(&count, 100000);


		bool success = true;
		for (int j = image->width - 1; j >= 0; j--){
			int val = 3 * (image->width - 1 - j);
			if ((values[i - val] & 0xff) != image->image_array[(0 * image->width * image->height) + j]){
				success = false;
				break;
			}
			if ((values[i - (val + 1)] & 0xff) != image->image_array[(1 * image->width * image->height) + j]){ /* this may need to be changed */
				success = false;
				break;
			}
			if ((values[i - (val + 2)] & 0xff) != image->image_array[(2 * image->width * image->height) + j]){ /* this may need to be changed */
				success = false;
				break;
			}

		}



		if (success){ /* verify the region actually has the image */
			vector<uint32_t> start_points;
			int start_local = 0;
			int last = 0;
			/* get the starting points of each image line */
			for (int j = i; j >= image_size; j--){

				bool found = true;
				for (int k = image->width - 1; k >= 0; k--){
					int val = 3 * (image->width - 1 - k);
					if ((values[j - (val)] & 0xff) != (uint8_t)(image->image_array[(0 * image->width * image->height) + last + k])){
						found = false;
						break;
					}
					if ((values[j - (val + 1)] & 0xff) != (uint8_t)image->image_array[(1 * image->width * image->height) + last + k]){
						found = false;
						break;
					}
					if ((values[j - (val + 2)] & 0xff) != (uint8_t)image->image_array[(2 * image->width * image->height) + last + k]){
						found = false;
						break;
					}
				}



				if (found){
					start_local = j;
					last += image->width;
					j -= 3 * (image->width) - 1; /* because of j--*/
					start_points.push_back(start_local);
					if (last == image->width * image->height) { break; }
				}

			}
			/* if we covered the entire image */
			if (last == image->width * image->height){


				/* check whether the gaps are uniform */
				vector<uint32_t> gaps;
				for (int j = 0; j < start_points.size() - 1; j++){
					gaps.push_back(start_points[j] - start_points[j + 1]);
				}

				uint32_t found = true;
				uint32_t comp_val = gaps[0];
				for (int j = 1; j < gaps.size(); j++){
					if (comp_val != gaps[j]){
						found = false; break;
					}
				}
				if (found){
					mem_regions_t * region = new mem_regions_t();

					region->bytes_per_pixel = 1;
					region->dimensions = 3;

					region->strides[0] = 1;
					region->strides[1] = 3;
					region->strides[2] = gaps[0];

					region->padding_filled = 1;
					if (gaps[0] != image->width * 3){
						region->padding[0] = gaps[0] - image->width * 3;
					}
					else region->padding[0] = 0;
					
					region->extents[0] = 3;
					region->extents[1] = image->width;
					region->extents[2] = image->height;

					/* region start and the end */
					region->start = start_points[0];
					region->end = start_points[start_points.size() - 1] - region->strides[2];
					*end = region->end;
					return region;
				}
					
				
			}
		}
	}

	return NULL;





}

vector<mem_regions_t *> get_image_regions_from_dump(vector<string> filenames, string in_image_filename, string out_image_filename){

	DEBUG_PRINT(("analyzing mem dumps....\n"), 1);
	DEBUG_PRINT(("get_image_regions_from_dump....\n"), 2);

	image_t * in_image = populate_imageinfo(open_image(in_image_filename.c_str()));
	image_t * out_image = populate_imageinfo(open_image(out_image_filename.c_str()));
	vector<mem_regions_t *> regions;

	bool similar = true;
	for (int i = 0; i < in_image->width * in_image->height; i++){
		if (in_image->image_array[i] != out_image->image_array[i]){
			DEBUG_PRINT(("the two images are not similar\n"), 2);
			similar = false; break;
			
		}
	}
	if (similar) DEBUG_PRINT(("the two images are not similar\n"), 2);
	
	for (int i = 0; i < filenames.size(); i++){

		DEBUG_PRINT(("analyzing file - %s\n", filenames[i].c_str()), 2);

		vector<string> parts = split(filenames[i], '_');
		uint32_t index = parts.size() - 2;
		bool write = parts[index][0] - '0';
		uint32_t size = strtoull(parts[index - 1].c_str(), NULL, 10);
		uint64_t base_pc = strtoull(parts[index - 2].c_str(), NULL, 16);

		DEBUG_PRINT(("analyzing - base_pc %llx, size %x, write %d\n", base_pc, size, write), 2);

		uint64_t start = 0;
		uint64_t end = 0;
		uint64_t start_back;
		uint64_t end_back = 0;

		mem_regions_t * mem = NULL;

		/* read the file into a buffer */
		struct stat results;
		char *  file_values;

		ASSERT_MSG(stat(filenames[i].c_str(), &results) == 0, ("file stat collection unsuccessful\n"));
		ifstream file(filenames[i].c_str(), ios::in | ios::binary);
		file_values = new char[results.st_size];
		file.read(file_values, results.st_size);

		start_back = results.st_size - 1;

		bool found_region = true;

		while (found_region){

			/* locate the image */
			if (write){
				mem = locate_image_CN2(file_values, results.st_size, &start, &end, out_image);
				if (mem == NULL){
					//mem = locate_image_CN2_backward(file_values, results.st_size, &start_back, &end_back, in_image);
					mem = locate_image_CN2_backward_write(file_values, results.st_size, &start_back, &end_back, out_image, 1);
				}
			}
			else{
				mem = locate_image_CN2(file_values, results.st_size, &start, &end, in_image);
				if(mem == NULL) mem = locate_image_CN2_backward(file_values, results.st_size, &start_back, &end_back, in_image);
			}

			start = end;
			start_back = end_back;

			if (mem != NULL){
				mem->dump_type = write ? OUTPUT_BUFFER : INPUT_BUFFER;
				mem->start += base_pc;
				mem->end += base_pc;
				DEBUG_PRINT(("region found(%u) - start %llx end %llx padding %u\n", write, mem->start, mem->end, mem->padding[0]), 2);
				regions.push_back(mem);
			}
			else{
				found_region = false;
			}
		}

		delete file_values;
		file.close();
	}

	
	/* remove duplicates */
	for (int i = 0; i < regions.size(); i++){
		for (int j = 0; j < i; j++){
			mem_regions_t * candidate = regions[j];
			mem_regions_t * check = regions[i];
			if (check->start == candidate->start && check->end == candidate->end){
				regions.erase(regions.begin() + i--);
				break;
			}
		}
	}

	DEBUG_PRINT(("no of mem regions found - %d\n", regions.size()), 2);
	DEBUG_PRINT(("get_image_regions_from_dump - done\n"), 2);

	return regions;

}

