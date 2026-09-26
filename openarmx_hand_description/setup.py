from glob import glob

from setuptools import find_packages, setup


package_name = "openarmx_hand_description"


setup(
    name=package_name,
    version="0.1.0",
    packages=find_packages(),
    data_files=[
        ("share/ament_index/resource_index/packages", [f"resource/{package_name}"]),
        (f"share/{package_name}", ["package.xml", "README.md", "README_CN.md", "LICENSE"]),
        (f"share/{package_name}/config", glob("config/*.yaml")),
        (
            f"share/{package_name}/meshes/arm/v10/visual",
            glob("meshes/arm/v10/visual/*"),
        ),
        (
            f"share/{package_name}/meshes/arm/v10/collision",
            glob("meshes/arm/v10/collision/*"),
        ),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    maintainer="OpenArmX",
    maintainer_email="openarmrobot@gmail.com",
    description="Composable OpenArmX bimanual description with O6 hands.",
    license="CC-BY-NC-SA-4.0",
)
