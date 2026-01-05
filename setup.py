from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension, build_ext

ext_modules = [
    Pybind11Extension(
        "mc_brb_module",
        ["Cpplibs/MaxClique/mc_brb.cpp"],
        extra_compile_args=["-O3", "-std=c++17"],
    )
]

setup(
    name="mc_brb_module",
    version="0.1",
    description="MC-BRB Maximum Clique pybind11 module",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
)
