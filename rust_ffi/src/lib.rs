#[allow(non_upper_case_globals)]
mod bindings;

use bindings::{
    pacparser_cleanup, pacparser_find_proxy, pacparser_init, pacparser_parse_pac_string,
};
use std::ffi::{CStr, CString};
use url::Url;

#[derive(Debug)]
pub struct PacParser {}

impl PacParser {
    pub fn new(pac_string: &str) -> Result<Self, String> {
        let pac = CString::new(pac_string).unwrap();

        if unsafe { pacparser_init() } != 1 {
            unsafe { pacparser_cleanup() };
            return Err("js context initialization failed!".to_owned());
        }
        if unsafe { pacparser_parse_pac_string(pac.as_ptr()) } != 1 {
            unsafe { pacparser_cleanup() };
            return Err("pac parse failed!".to_owned());
        }
        Ok(Self {})
    }

    pub fn find_proxy(&self, url: &str) -> Result<String, String> {
        let u: Url = Url::parse(url).map_err(|e| {
            println!("{:?}", e);
            "invalid url!"
        })?;
        let host = u.host_str().unwrap();
        let (u, l) = (
            CString::new(u.to_owned().into_string()).unwrap(),
            CString::new(host).unwrap(),
        );
        let res = unsafe {
            let s = pacparser_find_proxy(u.as_c_str().as_ptr(), l.as_c_str().as_ptr());
            CStr::from_ptr(s).to_string_lossy().into_owned()
        };

        Ok(res)
    }
}

impl Drop for PacParser {
    fn drop(&mut self) {
        unsafe { pacparser_cleanup() };
    }
}

#[test]
pub fn test() {
    let test_pac = std::fs::read_to_string("../tests/proxy.pac").unwrap();
    let p = PacParser::new(&test_pac).unwrap();

    assert_eq!(
        "END-OF-SCRIPT",
        p.find_proxy("http://www.google.co.in").unwrap()
    );
    assert_eq!(
        "plainhost/.manugarg.com",
        p.find_proxy("http://host1").unwrap()
    );

    assert_eq!(
        "plainhost/.manugarg.com",
        p.find_proxy("http://www1.manugarg.com").unwrap()
    );

    assert_eq!(
        "externaldomain",
        p.find_proxy("http://manugarg.externaldomain.com").unwrap()
    );

    assert_eq!(
        "isResolvable",
        p.find_proxy("http://www.google.com").unwrap()
    );

    assert_eq!(
        "secureUrl",
        p.find_proxy("https://www.somehost.com").unwrap()
    );
    assert_eq!(
        "END-OF-SCRIPT",
        p.find_proxy("http://www.somehost.com").unwrap()
    );
}
