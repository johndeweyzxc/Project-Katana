
def hex_to_ascii(hex_str: str):
    return bytearray.fromhex(hex_str.upper()).decode()


def ascii_to_hex(ascii_input: str):
    return ascii_input.encode('utf-8').hex().upper()


def hex_to_int_str(hex_str: str):
    return str(int(hex_str, 16))


def int_to_padded_hex(num):
    hex_str = format(num, 'x')
    return hex_str.zfill(2).upper()