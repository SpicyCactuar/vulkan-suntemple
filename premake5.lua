workspace "vksuntemple"
	language "C++"
	cppdialect "C++20"

	platforms { "x64" }
	configurations { "debug", "release" }

	flags "NoPCH"
	flags "MultiProcessorCompile"

	startproject "vksuntemple"

	debugdir "%{wks.location}"
	objdir "_build_/%{cfg.buildcfg}-%{cfg.platform}-%{cfg.toolset}"
	targetsuffix "-%{cfg.buildcfg}-%{cfg.platform}-%{cfg.toolset}"
	
	-- Default toolset options
	filter "toolset:gcc or toolset:clang"
		linkoptions { "-pthread" }
		buildoptions { "-march=native", "-Wall", "-pthread" }

	filter "toolset:msc-*"
		defines { "_CRT_SECURE_NO_WARNINGS=1" }
		defines { "_SCL_SECURE_NO_WARNINGS=1" }
		buildoptions { "/utf-8" }
	
	filter "*"

	-- default libraries
	filter "system:linux"
		links "dl"
	
	filter "system:windows"
		links { "opengl32", "gdi32" }

	filter "*"

	-- default outputs
	filter "kind:StaticLib"
		targetdir "lib/"

	filter "kind:ConsoleApp"
		targetdir "bin/"
		targetextension ".exe"
	
	filter "*"

	--configurations
	filter "debug"
		symbols "On"
		defines { "_DEBUG=1" }

	filter "release"
		optimize "On"
		defines { "NDEBUG=1" }

	filter "*"

-- Third party dependencies
include "third-party"

-- GLSLC helpers
dofile( "util/glslc.lua" )

-- Projects
project "vksuntemple"
	local sources = { 
		"vksuntemple/**.cpp",
		"vksuntemple/**.hpp",
		"vksuntemple/**.hxx"
	}

	kind "ConsoleApp"
	location "vksuntemple"

	files( sources )

	dependson "vksuntemple-shaders"

	links "vkutils"
	links "x-volk"
	links "x-stb"
	links "x-glfw"
	links "x-vma"

	dependson "x-glm" 

project "vksuntemple-shaders"
	local shaders = { 
		"vksuntemple/shaders/*.vert",
		"vksuntemple/shaders/*.frag",
		"vksuntemple/shaders/*.comp",
		"vksuntemple/shaders/*.geom",
		"vksuntemple/shaders/*.tesc",
		"vksuntemple/shaders/*.tese"
	}

	kind "Utility"
	location "vksuntemple/shaders"

	files( shaders )

	handle_glsl_files( "-O", "assets/vksuntemple/shaders", {} )

project "assets-bake"
	local sources = { 
		"assets-bake/**.cpp",
		"assets-bake/**.hpp",
		"assets-bake/**.hxx"
	}

	kind "ConsoleApp"
	location "assets-bake"

	files( sources )

	links "vkutils" -- for vkutils::Error
	links "x-tgen"
	links "x-zstd"

	dependson "x-glm" 
	dependson "x-rapidobj"

project "vkutils"
	local sources = { 
		"vkutils/**.cpp",
		"vkutils/**.hpp",
		"vkutils/**.hxx"
	}

	kind "StaticLib"
	location "vkutils"

	files( sources )

--EOF
