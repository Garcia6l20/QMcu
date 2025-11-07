from pathlib import Path

here = Path(__file__).parent
templates_path = here / "templates"
shaders_templates_path = templates_path / "shaders"
shaders_output_path = here / "shaders"

shaders_config = {
    "line-plot-series.vert.jinja2": {
        "line-plot-series-float.vert": {
            "ssbo_buffer_type": "float",
            "use_integral_converters": False,
            "decode_expression": "inData.data[byteIndex / 4]",
        },
        "line-plot-series-double.vert": {
            "ssbo_buffer_type": "double",
            "use_integral_converters": False,
            "decode_expression": "float(inData.data[byteIndex / 8])",
        },
        "line-plot-series-i8.vert": {
            "ssbo_buffer_type": "uint",
            "use_integral_converters": True,
            "decode_expression": "decodeI8(byteIndex)",
        },
        "line-plot-series-u8.vert": {
            "ssbo_buffer_type": "uint",
            "use_integral_converters": True,
            "decode_expression": "decodeU8(byteIndex)",
        },
        "line-plot-series-i16.vert": {
            "ssbo_buffer_type": "uint",
            "use_integral_converters": True,
            "decode_expression": "decodeI16(byteIndex)",
        },
        "line-plot-series-u16.vert": {
            "ssbo_buffer_type": "uint",
            "use_integral_converters": True,
            "decode_expression": "decodeU16(byteIndex)",
        },
        "line-plot-series-i32.vert": {
            "ssbo_buffer_type": "uint",
            "use_integral_converters": True,
            "decode_expression": "decodeI32(byteIndex)",
        },
        "line-plot-series-u32.vert": {
            "ssbo_buffer_type": "uint",
            "use_integral_converters": True,
            "decode_expression": "decodeU32(byteIndex)",
        },
    }
}

if __name__ == "__main__":
    import jinja2

    env = jinja2.Environment(loader=jinja2.FileSystemLoader(shaders_templates_path))
    for in_template, configs in shaders_config.items():
        t = env.get_template(in_template)
        for out_filename, config in configs.items():
            with open(shaders_output_path / out_filename, "w") as out:
                out.write(t.render(**config))
