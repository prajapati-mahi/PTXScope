import type { languages } from 'monaco-editor';

export const ptxLanguageDefinition: languages.IMonarchLanguage = {
  defaultToken: '',
  tokenPostfix: '.ptx',

  keywords: [
    'mov', 'add', 'sub', 'mul', 'mad', 'setp', 'ld', 'st', 'bra', 'ret', 'exit', 'bar'
  ],

  directives: [
    '.version', '.target', '.address_size', '.entry', '.func',
    '.param', '.reg', '.local', '.shared', '.global', '.const',
    '.visible', '.weak'
  ],

  types: [
    '.u8', '.u16', '.u32', '.u64',
    '.s8', '.s16', '.s32', '.s64',
    '.b8', '.b16', '.b32', '.b64',
    '.f16', '.f32', '.f64', '.pred'
  ],

  modifiers: [
    '.lo', '.hi', '.wide', '.approx', '.rn', '.rz', '.rm', '.rp',
    '.eq', '.ne', '.lt', '.le', '.gt', '.ge',
    '.uni', '.sync'
  ],

  tokenizer: {
    root: [
      // Directives
      [/\.[a-zA-Z_][a-zA-Z0-9_]*/, {
        cases: {
          '@directives': 'keyword.directive',
          '@types': 'type',
          '@modifiers': 'annotation',
          '@default': 'identifier'
        }
      }],

      // Predicates: @p0, @!p1
      [/@!?[a-zA-Z0-9_%]+/, 'variable.predicate'],

      // Special and general registers: %r1, %rd0, %tid.x
      [/%\b(tid|ntid|ctaid|nctaid)\.[xyz]\b/, 'variable.predefined'],
      [/%\b(laneid|warpid|clock|clock64)\b/, 'variable.predefined'],
      [/%\b[a-zA-Z0-9_\.]+\b/, 'variable.parameter'],

      // Instructions
      [/\b[a-z][a-z0-9_]*\b/, {
        cases: {
          '@keywords': 'keyword',
          '@default': 'identifier'
        }
      }],

      // Labels: label:
      [/^[a-zA-Z_][a-zA-Z0-9_]*:/, 'type.identifier'],
      [/[a-zA-Z_][a-zA-Z0-9_]*:/, 'type.identifier'],

      // Comments
      [/\/\/.*$/, 'comment'],
      [/\/\*/, 'comment', '@comment'],

      // Numbers
      [/0[xX][0-9a-fA-F]+/, 'number.hex'],
      [/0[fFdD][0-9a-fA-F]+/, 'number.float'],
      [/-?\d+(\.\d+)?([eE][\-+]?\d+)?f?/, 'number'],

      // Delimiters
      [/[{}()\[\]]/, '@brackets'],
      [/[,;:]/, 'delimiter'],
      [/[+\-]/, 'operator'],

      // Strings
      [/"([^"\\]|\\.)*"/, 'string'],
    ],

    comment: [
      [/[^\/*]+/, 'comment'],
      [/\*\//, 'comment', '@pop'],
      [/[\/*]/, 'comment']
    ],
  }
};

export const ptxTheme: any = {
  base: 'vs-dark',
  inherit: true,
  rules: [
    { token: 'keyword.directive', foreground: '38bdf8', fontStyle: 'bold' },
    { token: 'keyword', foreground: 'f43f5e', fontStyle: 'bold' },
    { token: 'type', foreground: 'a855f7' },
    { token: 'annotation', foreground: 'c084fc' },
    { token: 'variable.predicate', foreground: 'f59e0b', fontStyle: 'bold' },
    { token: 'variable.predefined', foreground: '10b981', fontStyle: 'bold' },
    { token: 'variable.parameter', foreground: 'e2e8f0' },
    { token: 'type.identifier', foreground: 'fbbf24', fontStyle: 'bold' },
    { token: 'comment', foreground: '64748b', fontStyle: 'italic' },
    { token: 'number', foreground: '34d399' },
    { token: 'number.hex', foreground: '34d399' },
    { token: 'number.float', foreground: '34d399' },
  ],
  colors: {
    'editor.background': '#090d16',
    'editor.foreground': '#f8fafc',
    'editorLineNumber.foreground': '#334155',
    'editorLineNumber.activeForeground': '#94a3b8',
    'editor.selectionBackground': '#1e293b',
    'editor.lineHighlightBackground': '#0f172a',
    'editorGutter.background': '#090d16',
  }
};
