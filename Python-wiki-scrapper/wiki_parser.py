import argparse
from wiki_controller import WikiController

class Parser:
    def __init__(self, wiki_link):
        self.link = wiki_link
        self.controller = WikiController(self.link)
        self.parser = argparse.ArgumentParser()
        self.prepare_parser()

    def prepare_parser(self):
        group = self.parser.add_mutually_exclusive_group(required=True)
        group.add_argument('--summary', type=str)
        group.add_argument('--table', type=str)
        group.add_argument('--count-words', type=str)
        group.add_argument('--analyze-relative-word-frequency', action="store_true")
        group.add_argument('--auto-count-words', type=str)

        self.parser.add_argument('--number', type=int)
        self.parser.add_argument('--first-row-is-header', action="store_true")
        self.parser.add_argument('--mode', type=str)
        self.parser.add_argument('--count', type=int)
        self.parser.add_argument('--chart', type=str)
        self.parser.add_argument('--depth', type=int)
        self.parser.add_argument('--wait', type=int)

    def handle_args(self):
        args = self.parser.parse_args()

        if args.summary:
            self.controller.summary(args.summary)
        elif args.table:
            self.controller.table(args.table, args.number, args.first_row_is_header)
        elif args.count_words:
            self.controller.count_words(args.count_words)
        elif args.analyze_relative_word_frequency:
            self.controller.analyze_relative_word_frequency(args.mode, args.count, args.chart)
        else:
            self.controller.auto_count_words(args.auto_count_words, args.depth, args.wait)


if __name__ == "__main__":
    link = "https://bulbapedia.bulbagarden.net/wiki"
    parser = Parser(link)
    parser.handle_args()