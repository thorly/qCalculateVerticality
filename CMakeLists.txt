# CloudCompare example for standard plugins

# REPLACE ALL 'ExamplePlugin' OCCURENCES BY YOUR PLUGIN NAME
# AND ADAPT THE CODE BELOW TO YOUR OWN NEEDS!

# Add an option to CMake to control whether we build this plugin or not
option( PLUGIN_STANDARD_CalculateVerticality "Install CalculateVerticality plugin" OFF )

if ( PLUGIN_STANDARD_CalculateVerticality )
	project( qCalculateVerticality )
	
	find_package( PCL 1.9 REQUIRED )
	find_package(Qt5 COMPONENTS Widgets PrintSupport REQUIRED)
	find_package(QXlsxQt5 REQUIRED)
	 
	AddPlugin( NAME ${PROJECT_NAME} )
		
	add_subdirectory( include )
	add_subdirectory( src )
	add_subdirectory( ui )
	
	target_include_directories( ${PROJECT_NAME}
		PUBLIC
			${PCL_INCLUDE_DIRS}
	)
	
	if( CMAKE_VERSION VERSION_LESS 3.13 )
		link_directories( ${PCL_LIBRARY_DIRS} )
	else()
		target_link_directories( ${PROJECT_NAME}
			PUBLIC
				${PCL_LIBRARY_DIRS}
		)
	endif()
	
	target_link_libraries( ${PROJECT_NAME}
		${PCL_LIBRARIES}
		QXlsx::QXlsx
		C:\\Qt\\Qt5.12.12\\5.12.12\\msvc2017_64\\lib\\Qt5PrintSupport.lib
	)
	
endif()
