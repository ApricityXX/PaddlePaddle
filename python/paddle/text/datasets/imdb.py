#   Copyright (c) 2020 PaddlePaddle Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
from __future__ import annotations

import collections
import re
import string
import tarfile
from typing import TYPE_CHECKING, Literal

import numpy as np

from paddle.dataset.common import _check_exists_and_download
from paddle.io import Dataset

if TYPE_CHECKING:
    from re import Pattern

    import numpy.typing as npt

    _ImdbDataSetMode = Literal["train", "test"]
__all__ = []

URL = 'https://dataset.bj.bcebos.com/imdb%2FaclImdb_v1.tar.gz'
MD5 = '7c2ac02c03563afcf9b574c7e56c153a'


class Imdb(Dataset):
    """
    Implementation of `IMDB <https://www.imdb.com/interfaces/>`_ dataset.

    Args:
        data_file(str|None): path to data tar file, can be set None if
            :attr:`download` is True. Default None.
        mode(str): 'train' 'test' mode. Default 'train'.
        cutoff(int): cutoff number for building word dictionary. Default 150.
        download(bool): whether to download dataset automatically if
            :attr:`data_file` is not set. Default True.

    Returns:
        Dataset: instance of IMDB dataset

    Examples:

        .. code-block:: python

            >>> # doctest: +TIMEOUT(75)
            >>> import paddle
            >>> from paddle.text.datasets import Imdb

            >>> class SimpleNet(paddle.nn.Layer):
            ...     def __init__(self):
            ...         super().__init__()
            ...
            ...     def forward(self, doc, label):
            ...         return paddle.sum(doc), label


            >>> imdb = Imdb(mode='train')

            >>> for i in range(10):
            ...     doc, label = imdb[i]
            ...     doc = paddle.to_tensor(doc)
            ...     label = paddle.to_tensor(label)
            ...
            ...     model = SimpleNet()
            ...     image, label = model(doc, label)
            ...     print(doc.shape, label.shape)
            [121] [1]
            [115] [1]
            [386] [1]
            [471] [1]
            [585] [1]
            [206] [1]
            [221] [1]
            [324] [1]
            [166] [1]
            [598] [1]

    """

    data_file: str | None
    mode: _ImdbDataSetMode
    word_idx: dict[str, int]
    docs: list
    labels: list

    def __init__(
        self,
        data_file: str | None = None,
        mode: _ImdbDataSetMode = 'train',
        cutoff: int = 150,
        download: bool = True,
    ) -> None:
        assert mode.lower() in [
            'train',
            'test',
        ], f"mode should be 'train', 'test', but got {mode}"
        self.mode = mode.lower()

        self.data_file = data_file
        if self.data_file is None:
            assert (
                download
            ), "data_file is not set and downloading automatically is disabled"
            self.data_file = _check_exists_and_download(
                data_file, URL, MD5, 'imdb', download
            )

        # Build a word dictionary from the corpus
        self.word_idx = self._build_work_dict(cutoff)

        # read dataset into memory
        self._load_anno()

    def _build_work_dict(self, cutoff: int) -> dict[str, int]:
        word_freq = collections.defaultdict(int)
        pattern = re.compile(r"aclImdb/((train)|(test))/((pos)|(neg))/.*\.txt$")
        for doc in self._tokenize(pattern):
            for word in doc:
                word_freq[word] += 1

        # Not sure if we should prune less-frequent words here.
        word_freq = [x for x in word_freq.items() if x[1] > cutoff]

        dictionary = sorted(word_freq, key=lambda x: (-x[1], x[0]))
        words, _ = list(zip(*dictionary))
        word_idx = dict(list(zip(words, range(len(words)))))
        word_idx['<unk>'] = len(words)
        return word_idx

    def _tokenize(self, pattern: Pattern[str]) -> list[list[str]]:
        data = []
        with tarfile.open(self.data_file) as tarf:
            tf = tarf.next()
            while tf is not None:
                if bool(pattern.match(tf.name)):
                    # newline and punctuations removal and ad-hoc tokenization.
                    data.append(
                        tarf.extractfile(tf)
                        .read()
                        .rstrip(b'\n\r')
                        .translate(None, string.punctuation.encode('latin-1'))
                        .lower()
                        .split()
                    )
                tf = tarf.next()

        return data

    def _load_anno(self) -> None:
        pos_pattern = re.compile(fr"aclImdb/{self.mode}/pos/.*\.txt$")
        neg_pattern = re.compile(fr"aclImdb/{self.mode}/neg/.*\.txt$")

        UNK = self.word_idx['<unk>']

        self.docs = []
        self.labels = []
        for doc in self._tokenize(pos_pattern):
            self.docs.append([self.word_idx.get(w, UNK) for w in doc])
            self.labels.append(0)
        for doc in self._tokenize(neg_pattern):
            self.docs.append([self.word_idx.get(w, UNK) for w in doc])
            self.labels.append(1)

    def __getitem__(
        self, idx: int
    ) -> tuple[npt.NDArray[np.int_], npt.NDArray[np.int_]]:
        return (np.array(self.docs[idx]), np.array([self.labels[idx]]))

    def __len__(self) -> int:
        return len(self.docs)
