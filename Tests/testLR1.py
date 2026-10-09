#!/usr/bin/env python3
"""Pruebas de caja negra del generador canonico y su parser."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
import sys
import unittest

BINARY = Path(__file__).resolve().parents[1] / 'Parser' / 'build' / 'lr1'


class LR1Tests(unittest.TestCase):
    def engine(self, grammar, tokens=None, extra=()):
        cmd = [str(BINARY), '--json', *extra]
        if tokens is not None:
            cmd += ['--input', tokens]
        proc = subprocess.run(cmd, input=grammar, text=True, capture_output=True, timeout=15)
        self.assertIn(proc.returncode, (0, 2, 3), proc.stderr)
        return json.loads(proc.stdout), proc.returncode

    def accepted(self, grammar, tokens):
        result, code = self.engine(grammar, tokens)
        self.assertEqual(code, 0, result.get('parse'))
        self.assertTrue(result['parse']['accepted'])
        return result

    def test_user_grammar(self):
        grammar = 'S -> G\nG -> A | B\nA -> ( A ) | a\nB -> ( B > | b'
        result = self.accepted(grammar, '( ( a ) )')
        self.assertEqual(len(result['states']), 19)  # includes generated S' -> S
        self.assertEqual(result['conflicts'], [])
        self.assertEqual(set(result['first']['G']), {'(', 'a', 'b'})
        self.accepted(grammar, '( ( b > >')
        self.accepted(grammar, 'a')
        self.accepted(grammar, 'b')
        for text in ('( a >', '( b )', '( a ) a', '', '( ( a )'):
            with self.subTest(text=text):
                rejected, code = self.engine(grammar, text)
                self.assertEqual(code, 3)
                self.assertFalse(rejected['parse']['accepted'])

    def test_lookahead_contexts(self):
        result, _ = self.engine('S -> G\nG -> A | B\nA -> ( A ) | a\nB -> ( B > | b')
        self.assertEqual({i['lookahead'] for i in result['states'][0]['closure']}, {'$'})
        index = next(i for i, p in enumerate(result['productions']) if p['lhs'] == 'A' and p['rhs'] == ['a'])
        completed = [(s['id'], i['lookahead']) for s in result['states'] for i in s['closure']
                     if i['production'] == index and i['dot'] == 1]
        self.assertEqual({la for _, la in completed}, {'$', ')'})
        self.assertEqual(len(completed), 2)
        for state, la in completed:
            self.assertEqual(result['action'][state], {la: [f'r{index}']})

    def test_epsilon_and_nullable_suffix(self):
        grammar = 'S -> A B C\nA -> a | ε\nB -> b | ε\nC -> c | ε'
        result = self.accepted(grammar, '')
        self.assertEqual(set(result['first']['S']), {'a', 'b', 'c', 'ε'})
        a_rules = {i for i, p in enumerate(result['productions']) if p['lhs'] == 'A'}
        lookaheads = {i['lookahead'] for i in result['states'][0]['closure'] if i['production'] in a_rules}
        self.assertEqual(lookaheads, {'b', 'c', '$'})
        for text in ('a', 'b', 'c', 'a b', 'a c', 'b c', 'a b c'):
            self.accepted(grammar, text)
        self.assertTrue(any(row['action'].endswith('ε') for row in result['parse']['trace']))

    def test_canonical_not_lalr(self):
        grammar = 'S -> a A d | b A e | a B e | b B d\nA -> c\nB -> c'
        data, _ = self.engine(grammar)
        self.assertTrue(data['lr1'])
        for text in ('a c d', 'b c e', 'a c e', 'b c d'):
            self.accepted(grammar, text)
        cores = {}
        for s in data['states']:
            key = frozenset((i['production'], i['dot']) for i in s['closure'])
            cores.setdefault(key, []).append(s)
        self.assertTrue(any(len(v) > 1 for v in cores.values()), 'LR(1) contexts must not be merged like LALR')

    def test_lr1_not_slr(self):
        grammar = 'S -> L = R | R\nL -> * R | id\nR -> L'
        self.accepted(grammar, 'id = * id')
        self.accepted(grammar, '* id')

    def test_shift_reduce_conflict_preserved(self):
        data, code = self.engine('E -> E + E | num', 'num + num + num')
        self.assertEqual(code, 2)
        self.assertTrue(any(c['type'] == 'shift/reduce' for c in data['conflicts']))
        self.assertEqual(data['parse']['trace'], [])
        for c in data['conflicts']:
            self.assertEqual(data['action'][c['state']][c['token']], c['actions'])
            self.assertGreater(len(c['actions']), 1)

    def test_reduce_reduce(self):
        data, code = self.engine('S -> A | B\nA -> a\nB -> a')
        self.assertEqual(code, 2)
        self.assertTrue(any(c['type'] == 'reduce/reduce' for c in data['conflicts']))

    def test_left_recursion_precedence(self):
        data = self.accepted('E -> E + T | T\nT -> T * F | F\nF -> ( E ) | num', 'num + num * num')
        reductions = [row['action'] for row in data['parse']['trace'] if row['action'].startswith('r')]
        multiply = next(i for i, action in enumerate(reductions) if 'T -> T * F' in action)
        addition = next(i for i, action in enumerate(reductions) if 'E -> E + T' in action)
        self.assertLess(multiply, addition)

    def test_trace_reduction_consumes_no_input(self):
        data = self.accepted('S -> ( A )\nA -> a', '( a )')
        rows = data['parse']['trace']
        for row, after in zip(rows, rows[1:]):
            self.assertEqual(len(row['states']), len(row['symbols']) + 1)
            if row['action'].startswith('r'):
                self.assertEqual(row['position'], after['position'])
            elif row['action'].startswith('s'):
                self.assertEqual(row['position'] + 1, after['position'])

    def test_arrow_numbering_continuations(self):
        grammar = "(0) S → A\n1. A -> a\n2. -> b\n | ε"
        for text in ('a', 'b', ''):
            self.accepted(grammar, text)

    def test_empty_spellings(self):
        for value in ('ε', 'eps', 'epsilon', "''", '""', ''):
            self.accepted(f'S -> {value}', '')

    def test_quoted_literals(self):
        self.accepted('S -> "|" "->" "epsilon" "two words"', '| -> epsilon "two words"')
        data = self.accepted('S -> "ε"', 'ε')
        self.assertEqual(data['nullable'], [])

    def test_automatic_augmentation_collision(self):
        data = self.accepted("S -> S'\nS' -> a", 'a')
        self.assertEqual(data['productions'][0]['lhs'], "S''")

    def test_end_marker(self):
        self.accepted('S -> a', 'a $')

    def test_duplicate_rules(self):
        data = self.accepted('S -> a | a\nS -> a', 'a')
        self.assertEqual(len(data['productions']), 2)

    def test_unknown_token_reports_expected(self):
        data, code = self.engine('S -> a', 'b')
        self.assertEqual(code, 3)
        self.assertIn('Esperado: a', data['parse']['message'])

    def test_step_limit_is_not_acceptance(self):
        data, code = self.engine('S -> a', 'a', ['--max-steps', '1'])
        self.assertEqual(code, 3)
        self.assertIn('Limite de pasos', data['parse']['message'])

    def test_invalid_grammars_and_limits(self):
        for grammar in ('', 'S a', '-> a', 'S -> a ε', 'S -> $', 'S -> "a', 'S -> "A"\nA -> a'):
            with self.subTest(grammar=grammar):
                p = subprocess.run([str(BINARY), '--json'], input=grammar, text=True, capture_output=True)
                self.assertEqual(p.returncode, 1)
                self.assertIn('Error:', p.stderr)
        p = subprocess.run([str(BINARY), '--max-states', '1'], input='S -> a', text=True, capture_output=True)
        self.assertEqual(p.returncode, 1)
        self.assertIn('Limite de estados', p.stderr)
        p = subprocess.run([str(BINARY), '--max-steps', '-1'], input='S -> a', text=True, capture_output=True)
        self.assertEqual(p.returncode, 1)

    def test_text_report(self):
        p = subprocess.run([str(BINARY), '--input', 'a'], input='S -> a', text=True, capture_output=True)
        self.assertEqual(p.returncode, 0)
        for heading in ('FIRST', 'Kernel:', 'Closure:', 'LR TABLE', 'SEGUIMIENTO', 'Entrada aceptada'):
            self.assertIn(heading, p.stdout)

    def test_unproductive_recursion_terminates(self):
        data, _ = self.engine('S -> A\nA -> A')
        self.assertLess(len(data['states']), 10)
        self.assertEqual(data['first']['S'], [])

    def test_small_language_exhaustive(self):
        # S generates exactly a* b*: compare with an independent language predicate.
        grammar = 'S -> A B\nA -> a A | ε\nB -> b B | ε'
        for n in range(5):
            for tokens in itertools.product('ab', repeat=n):
                data, _ = self.engine(grammar, ' '.join(tokens))
                self.assertEqual(data['parse']['accepted'], 'ba' not in ''.join(tokens), tokens)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--binary', type=Path, default=BINARY)
    options, rest = parser.parse_known_args()
    BINARY = options.binary.resolve()
    unittest.main(argv=[sys.argv[0], *rest])
