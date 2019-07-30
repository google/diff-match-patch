#!/usr/bin/env python
from setuptools import setup

setup(
    name='diff-match-patch',
    version='20180827',
    description='Diff Match Patch is a high-performance library in multiple languages that manipulates plain text.',
    packages = ['diff_match_patch'],
    author='Neil Fraser',
    author_email='fraser@google.com',
    url='https://github.com/google/diff-match-patch',
    test_suite='tests.diff_match_patch_test',
    license = 'Apache',
    classifiers = [
        "Topic :: Text Processing",
        "Intended Audience :: Developers",
        "Development Status :: 6 - Mature",
        "License :: OSI Approved :: Apache Software License",
        "Programming Language :: Python :: 2.6",
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.3",
        "Programming Language :: Python :: 3.4",
        "Programming Language :: Python :: 3.5",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
    ]
)
