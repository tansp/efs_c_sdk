#include <curl/curl.h>

#include "reader.h"

EFS_DLLAPI Efs_Error Efs_Rd_Reader_Open(Efs_Rd_Reader * rdr, const char * localFileName)
{
	Efs_Error err;
	err = Efs_File_Open(&rdr->file, localFileName);
	if (err.code != 200) {
		return err;
	} // if

	rdr->offset = 0;
	rdr->status = EFS_RD_OK;

	return Efs_OK;
}

EFS_DLLAPI void Efs_Rd_Reader_Close(Efs_Rd_Reader * rdr)
{
	Efs_File_Close(rdr->file);
}

EFS_DLLAPI size_t Efs_Rd_Reader_Callback(char * buffer, size_t size, size_t nitems, void * userData)
{
	ssize_t ret;
	Efs_Rd_Reader * rdr = (Efs_Rd_Reader *)userData;

	ret = Efs_File_ReadAt(rdr->file, buffer, size * nitems, rdr->offset);
	if (ret < 0) {
		rdr->status = EFS_RD_ABORT_BY_READAT;
		return CURL_READFUNC_ABORT;
	} // if

	if (rdr->abortCallback && rdr->abortCallback(rdr->abortUserData, buffer, ret)) {
		rdr->status = EFS_RD_ABORT_BY_CALLBACK;
		return CURL_READFUNC_ABORT;
	} // if

	rdr->offset += ret;
	return ret;
}
