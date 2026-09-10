add_rules("mode.debug", "mode.release")

add_requires("glfw", "glad", "glm", "embree", "nlohmann_json", "tinyobjloader")
add_requires("openvdb v13.0.0", {configs = {print = false}})
add_rules("plugin.compile_commands.autoupdate", {outputdir = "../.vscode"})
set_languages("c++23") 

target("HoRenderer")
    set_kind("binary")
    set_symbols("debug")
    set_strip("all") 
    add_files("src/*.cpp",
              "src/Core/*.cpp",
              "src/Common/*.cpp")
    
    add_headerfiles("src/Core/*.hpp",
                    "src/Common/*.hpp",
                    "src/Common/*.h")

    after_build(function (target)
        os.cp("Shader/*.vert", target:targetdir())
        os.cp("Shader/*.frag", target:targetdir())
    end)

    add_packages("glfw", "glad", "glm", "embree", "nlohmann_json", "tinyobjloader", "openvdb")

    on_load(function (target)
        if target:is_plat("macosx") then
            -- xmake packages such as Embree/TBB use @rpath on macOS.
            local package_linkdirs = target:get_from("linkdirs", "package::*")
            if package_linkdirs then
                for _, linkdirs in ipairs(package_linkdirs) do
                    for _, linkdir in ipairs(table.wrap(linkdirs)) do
                        target:add("rpathdirs", linkdir)
                    end
                end
            end
        end
    end)

    if is_plat("windows") then
        add_cxflags("/openmp:llvm")
    elseif is_plat("macosx") then
        -- Apple Clang needs the LLVM OpenMP runtime explicitly enabled.
        add_defines("GL_SILENCE_DEPRECATION")
        add_frameworks("Cocoa", "IOKit", "OpenGL")
        add_cxflags("-Xpreprocessor", "-fopenmp")
        add_links("omp")

        local libomp_prefix = os.getenv("LIBOMP_PREFIX")
        if not libomp_prefix or not os.isdir(libomp_prefix .. "/include") then
            if os.isdir("/opt/homebrew/opt/libomp/include") then
                libomp_prefix = "/opt/homebrew/opt/libomp"
            elseif os.isdir("/usr/local/opt/libomp/include") then
                libomp_prefix = "/usr/local/opt/libomp"
            end
        end

        if libomp_prefix and os.isdir(libomp_prefix .. "/include") and os.isdir(libomp_prefix .. "/lib") then
            add_includedirs(libomp_prefix .. "/include")
            add_linkdirs(libomp_prefix .. "/lib")
        end
    end

--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro definition
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--
