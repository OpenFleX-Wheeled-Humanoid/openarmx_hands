from setuptools import setup

package_name = "openarmx_hand_gui"

setup(
    name=package_name,
    version="0.0.0",
    packages=["gui"],
    data_files=[
        ("share/ament_index/resource_index/packages", [f"resource/{package_name}"]),
        (f"share/{package_name}", ["package.xml", "README.md", "README_CN.md"]),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    maintainer="openarmx",
    maintainer_email="openarmrobot@gmail.com",
    description="Common OpenArmX hands tools.",
    license="CC-BY-NC-SA-4.0",
    entry_points={
        "console_scripts": [
            "o6_manual_control_gui = gui.o6_manual_control_gui:main",
        ],
    },
)
