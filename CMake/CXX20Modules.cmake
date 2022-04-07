include(CMake/CXX20Modules/${CMAKE_CXX_COMPILER_ID}.cmake)

set(REGEX_WHITESPACE "[ \t]+")
set(REGEX_ID "[a-zA-Z_][a-zA-Z0-9_.]*")
set(REGEX_FILE "[a-zA-Z0-9_./\:]*")
set(REGEX_HEADER "(<${REGEX_FILE}>|\"${REGEX_FILE}\")")
set(REGEX_NAME "${REGEX_ID}(:${REGEX_ID})?")
set(REGEX_MODULE "(^|[\n\r])(export${REGEX_WHITESPACE})?module${REGEX_WHITESPACE}")
set(REGEX_IMPORT "(^|[\n\r])(export${REGEX_WHITESPACE})?import${REGEX_WHITESPACE}")

function(invoke_preprocessor
	file_name
	out_preprocessed_file
)
	file(REAL_PATH
		${file_name}
		file_path
	BASE_DIRECTORY
		${CMAKE_CURRENT_SOURCE_DIR}
		EXPAND_TILDE
	)

	file(READ
		${file_path}
		file_content
	)

	#module declarations and imports
	string(
	REGEX MATCHALL
		"(${REGEX_MODULE}${REGEX_NAME}|${REGEX_IMPORT}(:?${REGEX_ID}|${REGEX_HEADER}))"
		file_content
		${file_content}
	)

	set(${out_preprocessed_file} "${file_content}" PARENT_SCOPE)

endfunction()

function(read_module_name
	module_source_file
	preprocessed_file
	must_have_module_name
	out_module_name
	out_module_target_name
	out_module_binary
)
	string(
	REGEX MATCH
		"${REGEX_MODULE}${REGEX_NAME}"
		module_name
		"${preprocessed_file}"
	)
	list(TRANSFORM module_name REPLACE "${REGEX_MODULE}" "")

	if	(NOT module_name AND ${must_have_module_name})
		message(FATAL_ERROR "File '${module_source_file}' did not specify a valid module name!")
	endif()

	set(${out_module_name} ${module_name} PARENT_SCOPE)
	string(REPLACE ":" "-" module_target_name "${module_name}")
	set(${out_module_target_name} ${module_target_name} PARENT_SCOPE)
	set(${out_module_binary} ${PREBUILT_MODULE_PATH}/${module_target_name}${MODULE_INTERFACE_EXTENSION} PARENT_SCOPE)

endfunction()

function(read_module_headerunits
	preprocessed_file
	out_module_headerunits
)
	string(REGEX MATCHALL "${REGEX_IMPORT}${REGEX_HEADER}" module_dependencies "${preprocessed_file}")
	list(TRANSFORM module_dependencies REPLACE "${REGEX_IMPORT}(<|\")" "")
	list(TRANSFORM module_dependencies REPLACE "(>|\")" "")

	set(${out_module_headerunits} ${module_dependencies} PARENT_SCOPE)

endfunction()

function(read_module_dependencies
	preprocessed_file
	module_name
	out_module_dependencies
	out_module_target_dependencies
	out_module_dependency_binaries
)
	string(REGEX MATCHALL "${REGEX_IMPORT}${REGEX_ID}" dependencies "${preprocessed_file}")
	list(TRANSFORM dependencies REPLACE "${REGEX_IMPORT}" "")


	string(REGEX MATCHALL "${REGEX_IMPORT}:${REGEX_ID}" module_partitions "${preprocessed_file}")

	list(TRANSFORM module_partitions REPLACE "${REGEX_IMPORT}" "")
	list(TRANSFORM module_partitions PREPEND "${module_name}")

	list(APPEND dependencies ${module_partitions})
	set(${out_module_dependencies} ${dependencies} PARENT_SCOPE)

	set(module_target_dependencies ${dependencies})
	list(
		TRANSFORM
		module_target_dependencies
	REPLACE
		":"
		"-"
	)
	set(${out_module_target_dependencies} ${module_target_dependencies} PARENT_SCOPE)

	set(dependency_binaries ${module_target_dependencies})
	list(TRANSFORM dependency_binaries PREPEND ${PREBUILT_MODULE_PATH}/)
	list(TRANSFORM dependency_binaries APPEND ${MODULE_INTERFACE_EXTENSION})
	set(${out_module_dependency_binaries} ${dependency_binaries} PARENT_SCOPE)

endfunction()

function(add_module_source_dependencies
	target_name
	source_file
	module_dependencies
	module_target_dependencies
	module_dependency_files
)
	list(LENGTH module_dependencies module_dependency_count)
	if(${module_dependency_count} GREATER 0)

		add_dependencies(
		"${target_name}"
			${module_target_dependencies}
		)

		set_source_files_properties(
		"${source_file}"
		OBJECT_DEPENDS
			"${module_dependency_files}"
		)
	endif()

endfunction()

function(add_module_command
	module_interface_file
	module_binary
	module_name
	module_includes
	module_dependency_binaries
	module_byproducts
)

	get_compile_module_interface_command(
		${module_interface_file}
		${module_binary}
		"${MODULE_INCLUDES}"
		compile_module_interface_command
	)

	add_custom_command(
	OUTPUT
		${module_binary}
	COMMAND
		${compile_module_interface_command}
	VERBATIM
	MAIN_DEPENDENCY
		${module_interface_file}
	DEPENDS
		${module_dependency_binaries}
	BYPRODUCTS
		${MODULE_BYPRODUCTS}
	COMMENT
		"Generating precompiled module ${module_name}"
	)

endfunction()

function(add_module_partition
	module_name
	partition_name
	partition_target_name
	partition_binary
	interface_file
	preprocessed_module_file
	module_includes
	module_byproducts
)
	add_library(
		${partition_target_name}
	INTERFACE
		${interface_file}
	)
	read_module_dependencies(
		"${preprocessed_module_file}"
		${module_name}
		module_dependencies
		module_target_dependencies
		module_dependency_binaries
	)

	add_module_source_dependencies(
		${partition_target_name}
		${interface_file}
		"${module_dependencies}"
		"${module_target_dependencies}"
		"${module_dependency_binaries}"
	)

	add_module_command(
		${interface_file}
		${partition_binary}
		${partition_name}
		"${module_includes}"
		"${module_dependency_binaries}"
		"${module_byproducts}"
	)

	read_module_headerunits(
		"${preprocessed_module_file}"
		module_header_units
	)
	add_module_source_header_units(
		${module_name}
		${interface_file}
		"${module_header_units}"
	)


endfunction()

function(add_module_implementation
	source_file
	module_name
	module_target_name
	module_binary
	module_includes
	out_module_target_name
)
	invoke_preprocessor(
		"${source_file}"
		preprocessed_module_file
	)
	read_module_name(
		"${source_file}"
		"${preprocessed_module_file}"
		TRUE
		module_implementation_name
		module_implementation_target_name
		module_implementation_binary
	)
	read_module_dependencies(
		"${preprocessed_module_file}"
		"${module_name}"
		module_dependencies
		module_target_dependencies
		module_dependency_binaries
	)

	if	("${module_implementation_name}" STREQUAL "${module_name}")

		target_sources(
			"${module_target_name}+"
		PRIVATE
			${source_file}
		)

	else()
		add_library(
			"${module_implementation_target_name}"
		OBJECT
			"${source_file}"
		)

		target_compile_options(
			"${module_implementation_target_name}"
		PRIVATE
			${WARNING_FLAGS}
			${MODULE_FLAGS}
			${ADDITIONAL_COMPILE_OPTIONS}
		)

		target_include_directories(
			"${module_implementation_target_name}"
		PRIVATE
			"${module_includes}"
		)

		target_sources(
			"${module_target_name}+"
		PRIVATE
			$<TARGET_OBJECTS:${module_implementation_target_name}>
		)
	endif()

	add_module_source_dependencies(
		"${module_target_name}"
		"${source_file}"
		"${module_dependencies}"
		"${module_target_dependencies}"
		"${module_dependency_binaries}"
	)

	set("${out_module_target_name}" ${module_target_name} PARENT_SCOPE)

endfunction()

function(add_module
	module_interface_file
)
	cmake_parse_arguments(
		MODULE
		""
		""
		"PARTITION;IMPLEMENTATION;BYPRODUCTS;INCLUDES"
		${ARGN}
	)

	invoke_preprocessor(
		${module_interface_file}
		preprocessed_module_file
	)
	read_module_name(
		${module_interface_file}
		"${preprocessed_module_file}"
		TRUE
		module_name
		module_target_name
		module_binary
	)

	foreach(partition_file IN LISTS MODULE_PARTITION)
		invoke_preprocessor(
			"${partition_file}"
			preprocessed_partition_file
		)
		read_module_name(
			"${partition_file}"
			"${preprocessed_partition_file}"
			TRUE
			partition_name
			partition_target_name
			partition_binary
		)

		add_module_partition(
			"${module_name}"
			"${partition_name}"
			"${partition_target_name}"
			"${partition_binary}"
			"${partition_file}"
			"${preprocessed_partition_file}"
			"${MODULE_INCLUDES}"
			"${MODULE_BYPRODUCTS}"
		)
	endforeach()

	add_module_partition(
		${module_name}
		${module_name}
		${module_target_name}
		${module_binary}
		${module_interface_file}
		"${preprocessed_module_file}"
		"${MODULE_INCLUDES}"
		"${MODULE_BYPRODUCTS}"
	)

	if	(NOT "${MODULE_IMPLEMENTATION}" STREQUAL "")

		add_library(
			"${module_target_name}+"
		STATIC
			${MODULE_IMPLEMENTATION}
		)

		foreach(module_implementation IN LISTS MODULE_IMPLEMENTATION)
			add_module_implementation(
				"${module_implementation}"
				"${module_name}"
				"${module_target_name}"
				"${module_binary}"
				"${MODULE_INCLUDES}"
				module_implementation_target_name
			)
		endforeach()

		target_compile_options(
			"${module_target_name}+"
		PRIVATE
			-fmodule-file=${module_binary}
			${WARNING_FLAGS}
			${MODULE_FLAGS}
			${ADDITIONAL_COMPILE_OPTIONS}
		)

		target_include_directories(
			"${module_target_name}+"
		PRIVATE
			"${MODULE_INCLUDES}"
		)

		add_dependencies(
		"${module_target_name}+"
			"${module_target_name}"
		)

		target_link_libraries(
			${module_target_name}
		INTERFACE
			"${module_target_name}+"
		)
	endif()


endfunction()

function(add_module_dependencies
	target_name
)
	get_target_property(target_source_files
		${target_name}
		SOURCES
	)

	foreach(source_file IN LISTS target_source_files)

		invoke_preprocessor(
			${source_file}
			preprocessed_module_file
		)
		read_module_name(
			${source_file}
			"${preprocessed_module_file}"
			FALSE
			module_name
			module_target_name
			module_binary
		)
		read_module_dependencies(
			"${preprocessed_module_file}"
			"${module_name}"
			module_dependencies
			module_target_dependencies
			module_dependency_binaries
		)
		add_module_source_dependencies(
			"${target_name}"
			${source_file}
			"${module_dependencies}"
			"${module_target_dependencies}"
			"${module_dependency_binaries}"
		)

		#add a link dependency for non-interface libraries
		foreach(dependency IN LISTS module_dependencies)
			if	(TARGET "${dependency}+")
				target_link_libraries(
					${target_name}
					"${dependency}+"
				)
			endif()
		endforeach()

	endforeach()
endfunction()

function(add_module_object_library
	target_name
)
	add_library(
		${target_name}
	OBJECT
		${ARGN}
	)

	add_module_dependencies(
		${target_name}
	)

endfunction()

function(add_module_executable
	target_name
)
	add_executable(
		${target_name}
		${ARGN}
	)

	add_module_dependencies(
		${target_name}
	)

endfunction()
