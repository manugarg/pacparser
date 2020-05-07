use bindgen;
use std::path::PathBuf;

fn main() {
    println!("cargo:rustc-link-search=native=./libs");
    println!("cargo:rustc-link-lib=static=pacparser");
    println!("cargo:rustc-link-lib=static=js");

    //uses vfprint which is no longer in the standard library in MSVC2019
    #[cfg(target_os = "windows")]
    println!("cargo:rustc-link-lib=static=legacy_stdio_definitions");
    #[cfg(target_os = "windows")]
    println!(
        r#"cargo:rustc-link-search=native=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Tools\MSVC\14.25.28610\lib\x64"#
    );

    let bindings = bindgen::Builder::default()
        .header("includes/pacparser.h")
        .whitelist_function("pacparser_cleanup")
        .whitelist_function("pacparser_find_proxy")
        .whitelist_function("pacparser_init")
        .whitelist_function("pacparser_parse_pac_string")
        .generate()
        .unwrap();

    let out_path = PathBuf::from("./src");
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .unwrap();
}
