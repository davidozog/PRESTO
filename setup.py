from setuptools import setup
import os

# Utility function to read the README file.
# Used for the long_description.  It's nice, because now 1) we have a top level
# README file and 2) it's easier to type in the README file than to put a raw
# string in below ...
def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

setup(
    name = "MATLAByrinth",
    version = "0.0.1",
    author = "David Ozog",
    author_email = "ozog@uoregon.edu",
    description = ("A distributed Master/Worker framework for Matlab"),
    license = "BSD",
    keywords = "master worker matlab distributed parallel",
    url = "http://ix.cs.uoregon.edu/~ozog",
    packages=['MATLAByrinth'],
    long_description=read('README'),
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Topic :: Utilities",
        "License :: BSD License",
    ],
)
