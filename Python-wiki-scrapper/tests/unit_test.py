import pytest
import sys
from pathlib import Path

import pandas as pd

sys.path.append(str(Path(__file__).resolve().parent.parent))

from wiki_scrapper import WikiScrapper
from wiki_controller import WikiController
from wiki_errors import InvalidArgumentError

@pytest.fixture
def scrapper():
    return WikiScrapper("")

@pytest.fixture
def local_file():
    return "tests/Team_Rocket.html"

@pytest.fixture
def controller():
    return WikiController("")

def test_table(scrapper, local_file):
    scrapper.scrape(local_file=local_file)
    df = scrapper.get_table(1, first_headers=True)
    
    assert isinstance(df, pd.DataFrame)
    assert not df.empty
    
def test_summary(scrapper, local_file):
    scrapper.scrape(local_file=local_file)
    summary = scrapper.get_summary()

    assert isinstance(summary, str)
    assert len(summary) == 238

def test_open_local_file_that_doesnt_exists(scrapper):
    with pytest.raises(FileNotFoundError):
        scrapper.scrape(local_file="maslo.html")

def test_invalid_mode(controller):
    with pytest.raises(InvalidArgumentError):
        controller.analyze_relative_word_frequency("invalid mode", 10)

def test_invalid_count(controller):
    with pytest.raises(InvalidArgumentError):
        controller.analyze_relative_word_frequency("article", -10)