workspace "d3"
   configurations { "Debug", "Release" }

project "d3c"
   kind "ConsoleApp"
   language "C++"
   targetdir "../bin/%{cfg.buildcfg}"

   files { "../compier/**.h", "../compiler/**.cpp" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"

project "d3"
   kind "ConsoleApp"
   language "C"
   targetdir "../bin/%{cfg.buildcfg}"

   files { "../engine/c/**.h", "../engine/c/**.c" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
