add_requires("libssh")
add_requires("fmt")
add_requires("openssl")

if is_plat("macosx") then
    set_arch("x86_64")
end

target("sftp_pip")
    set_languages("cxx17")
    add_files(
        "src/sftp_pip.cc",
        "src/sftp_pip_impl.cc"
    )
    if is_plat("macosx") then
        set_arch("x86_64")
    end
    add_packages("libssh")
    add_packages("openssl")
    add_packages("fmt")
    after_build(function (target)
        -- Get the target file path (e.g., build output)
        local targetfile = target:targetfile()
        -- Copy to the project root (current directory)
        os.cp(targetfile, ".")
    end)
