use bindgen;
use std::path::PathBuf;

fn main() {
    println!("cargo:rustc-link-search=native=./libs");
    println!("cargo:rustc-link-lib=static=pacparser");
    println!("cargo:rustc-link-lib=static=js");

    //uses vfprint which is no longer in the standard library in MSVC2019
    #[cfg(target_os = "windows")]
    println!("cargo:rustc-link-lib=static=legacy_stdio_definitions");

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
