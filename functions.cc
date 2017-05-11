

uint64_t  VersionSet::NewFileNumber() {
	//printf("version_set.h, enter NewFileNumber\n");
		
		
		//return next_file_number_++;
		//get_file_name("sf");
		//AllocSeg(343);
		//printf("in version_set.cc, before alloc, file_number_=%d\n",next_file_number_);
		uint64_t finalNumber = Alloc_slot(next_file_number_);
		//printf("in version_set.cc, after alloc, finalNumber=%d\n",finalNumber );
		next_file_number_ = finalNumber+1;
		return finalNumber;
}