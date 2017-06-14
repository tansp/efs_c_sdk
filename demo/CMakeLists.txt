add_executable (put_file put_file.c)
target_link_libraries (put_file efs crypto m)

add_executable (put_stream put_stream.c)
target_link_libraries (put_stream efs crypto m)

add_executable (cdn_refresh cdn_refresh.c)
target_link_libraries (cdn_refresh efs crypto m)

add_executable (cdn_prefetch cdn_prefetch.c)
target_link_libraries (cdn_prefetch efs crypto m)

add_executable (cdn_flux cdn_flux.c)
target_link_libraries (cdn_flux efs crypto m)

add_executable (cdn_bandwidth cdn_bandwidth.c)
target_link_libraries (cdn_bandwidth efs crypto m)

add_executable (cdn_loglist cdn_loglist.c)
target_link_libraries (cdn_loglist efs crypto m)

add_executable (demo demo.c)
target_link_libraries (demo efs crypto m)

add_executable (put_file_abortable put_file_abortable.c)
target_link_libraries (put_file_abortable efs crypto m)

add_executable (gen_download_url_with_deadline gen_download_url_with_deadline.c)
target_link_libraries (gen_download_url_with_deadline efs crypto m)
