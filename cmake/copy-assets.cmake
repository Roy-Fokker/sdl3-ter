#---------------------------------------------------------------------------------------
# Function to take asset/data file and copy it as dependency of program

# TODO: Check if there is a better way to do this, cmake's install command maybe?

function(target_data_assets TARGET)
	# get count of asset files
	list(LENGTH ARGN count_files)

	# output directory
	set(data_dir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data)

	# loop over all the files
	foreach(file_name IN ITEMS ${ARGN})
		# get absolute path of file
		file(REAL_PATH ${file_name} source_abs)

		# copied file path
		set(output_file ${data_dir}/${file_name})

		# call copy command
		add_custom_command(
			OUTPUT ${output_file}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${data_dir}
			COMMAND ${CMAKE_COMMAND} -E copy ${source_abs} ${data_dir}
			DEPENDS ${source_abs}
			COMMENT "Copy ${file_name} to ${data_dir}"
		)

		list(APPEND data_sources ${file_name})
		list(APPEND data_outputs ${output_file})
	endforeach()

	# add custom target for all the copy operations
	add_custom_target(${TARGET}_DATA
		DEPENDS ${data_outputs}
		SOURCES ${data_sources}
	)

	# add custom target as a dependency to TARGET
	add_dependencies("${TARGET}" ${TARGET}_DATA)
endfunction()
