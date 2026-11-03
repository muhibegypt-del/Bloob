//! Converts one Dolby Vision RPU (HEVC NAL unit type 62) from a profile 7 disc
//! stream to profile 8.1, so a two-layer disc remux plays as single-layer
//! Dolby Vision: the enhancement layer is dropped by the caller and each RPU is
//! rewritten here (dovi_tool's "mode 2"). The work is the dolby_vision crate's
//! (MIT, quietvoid); this file only gives it a C face.
use dolby_vision::rpu::dovi_rpu::DoviRpu;
use dolby_vision::rpu::ConversionMode;

/// `nal`: the NAL unit without its start code, header (0x7C 0x01) included.
/// On success writes the converted NAL unit (header included, emulation
/// prevention applied) to `*out`, which the caller frees with
/// `bloob_dovi_free(*out, len)`, and returns its length. Returns 0 when the
/// unit is not a usable RPU; nothing is allocated then.
#[no_mangle]
pub unsafe extern "C" fn bloob_dovi_to81(nal: *const u8, len: usize, out: *mut *mut u8) -> usize {
    if nal.is_null() || out.is_null() || len < 8 {
        return 0;
    }
    let data = std::slice::from_raw_parts(nal, len);
    let mut rpu = match DoviRpu::parse_unspec62_nalu(data) {
        Ok(r) => r,
        Err(_) => return 0,
    };
    if rpu.convert_with_mode(ConversionMode::To81).is_err() {
        return 0;
    }
    let bytes = match rpu.write_hevc_unspec62_nalu() {
        Ok(b) => b,
        Err(_) => return 0,
    };
    let boxed = bytes.into_boxed_slice();
    let n = boxed.len();
    *out = Box::into_raw(boxed) as *mut u8;
    n
}

/// The profile a parsed RPU says it carries (7, 8, 5…), or -1 when unreadable.
#[no_mangle]
pub unsafe extern "C" fn bloob_dovi_profile(nal: *const u8, len: usize) -> i32 {
    if nal.is_null() || len < 8 {
        return -1;
    }
    let data = std::slice::from_raw_parts(nal, len);
    match DoviRpu::parse_unspec62_nalu(data) {
        Ok(r) => r.dovi_profile as i32,
        Err(_) => -1,
    }
}

#[no_mangle]
pub unsafe extern "C" fn bloob_dovi_free(p: *mut u8, len: usize) {
    if !p.is_null() {
        drop(Box::from_raw(std::ptr::slice_from_raw_parts_mut(p, len)));
    }
}
