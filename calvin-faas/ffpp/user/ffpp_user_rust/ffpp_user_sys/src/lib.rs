pub(crate) use ffpp_ffi;
use std::ffi::{CStr, CString};
use std::os::raw;
use std::ptr;

pub(crate) trait ToCString {
    fn to_cstring(self) -> CString;
}

impl ToCString for String {
    #[inline]
    fn to_cstring(self) -> CString {
        CString::new(self).unwrap()
    }
}

impl ToCString for &str {
    #[inline]
    fn to_cstring(self) -> CString {
        CString::new(self).unwrap()
    }
}

pub fn eal_init(args: Vec<String>) -> i32 {
    let len = args.len() as raw::c_int;
    let args = args.into_iter().map(|s| s.to_cstring()).collect::<Vec<_>>();
    let mut ptrs = args
        .iter()
        .map(|s| s.as_ptr() as *mut raw::c_char)
        .collect::<Vec<_>>();
    unsafe { ffpp_ffi::rte_eal_init(len, ptrs.as_mut_ptr()) }
}

pub fn eal_cleanup() -> i32 {
    unsafe { ffpp_ffi::rte_eal_cleanup() }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn eal_env() {
        eal_init(vec![
            "eal_test".to_string(),
            "-l".to_string(),
            "0".to_string(),
        ]);
        eal_cleanup();
    }
}
