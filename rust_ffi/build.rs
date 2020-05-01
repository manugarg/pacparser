use bindgen;
use std::path::PathBuf;

fn main() {
    println!("cargo:rustc-link-search=native=./libs");
    println!("cargo:rustc-link-lib=static=pacparser");
    println!("cargo:rustc-link-lib=static=js");
    let bindings = bindgen::Builder::default()
        .header("includes/pacparser.h")
        .derive_default(true)
        .derive_debug(true)
        .derive_eq(true)
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
