

uint64_t  VersionSet::NewFileNumber() {
	
		uint64_t finalNumber = Alloc_slot(next_file_number_);
		next_file_number_ = finalNumber+1;
		return finalNumber;
}