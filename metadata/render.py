#!/usr/bin/env python3
"""Render an ISO 19115-1 / 19115-3 metadata record for one edition of the
NZ Cost-Effective Land Cover series.

    python3 render.py 2024-25.yaml          # -> build/<slug>_iso19115-3.xml
    python3 render.py --all                 # render every edition
    python3 render.py 2024-25.yaml --check  # render and report only, no write

Each edition file in editions/ is deep-merged over common.yaml (series-level
facts; the edition file wins on any key it defines), and every string value in
the merged data is then passed through Jinja with the merged data as context,
so prose can carry placeholders like {{ edition }} or
{{ scalars.temporal_begin | nzdate }} instead of hard-coded per-edition values.
Placeholders must resolve in one pass: they may reference literal values only,
not other placeholder-bearing prose.

Requires: jinja2, pyyaml, lxml.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from xml.sax.saxutils import escape

import yaml
from jinja2 import Environment, FileSystemLoader, StrictUndefined
from lxml import etree

ROOT = Path(__file__).parent
TEMPLATE_DIR = ROOT / 'template'
EDITION_DIR = ROOT / 'editions'
COMMON_FILE = ROOT / 'common.yaml'
BUILD_DIR = ROOT / 'build'

# Expected order of MD_Metadata children per ISO 19115-1:2014. A validator will
# reject any other order, and schema validation is not available offline, so the
# order is asserted here instead.
MDB_ORDER = [
    'metadataIdentifier', 'defaultLocale', 'parentMetadata', 'metadataScope', 'contact', 'dateInfo',
    'metadataStandard', 'metadataProfile', 'alternativeMetadataReference', 'otherLocale',
    'metadataLinkage', 'spatialRepresentationInfo', 'referenceSystemInfo',
    'metadataExtensionInfo', 'identificationInfo', 'contentInfo', 'distributionInfo',
    'dataQualityInfo', 'resourceLineage', 'portrayalCatalogueInfo', 'metadataConstraints',
    'applicationSchemaInfo', 'metadataMaintenance', 'acquisitionInformation',
]

# Expected order of MD_Metadata children per ISO 19115:2003, for the companion
# ISO 19139 (gmd:) rendering consumed by the LRIS Portal / Koordinates.
GMD_ORDER = [
    'fileIdentifier', 'language', 'characterSet', 'parentIdentifier', 'hierarchyLevel',
    'hierarchyLevelName', 'contact', 'dateStamp', 'metadataStandardName', 'metadataStandardVersion',
    'dataSetURI', 'locale', 'spatialRepresentationInfo', 'referenceSystemInfo',
    'metadataExtensionInfo', 'identificationInfo', 'contentInfo', 'distributionInfo',
    'dataQualityInfo', 'portrayalCatalogueInfo', 'metadataConstraints', 'applicationSchemaInfo',
    'metadataMaintenance',
]

MONTHS = ['January', 'February', 'March', 'April', 'May', 'June', 'July',
          'August', 'September', 'October', 'November', 'December']


def nzdate(iso: str) -> str:
    """'2023-11-01' -> '1 November 2023'. Locale-independent."""
    y, m, d = str(iso).split('-')
    return f'{int(d)} {MONTHS[int(m) - 1]} {int(y)}'


def make_env() -> Environment:
    env = Environment(
        loader=FileSystemLoader(TEMPLATE_DIR),
        undefined=StrictUndefined,   # fail loudly on a missing key, never silently
        keep_trailing_newline=True,
        trim_blocks=False,
        lstrip_blocks=False,
    )
    env.filters['xe'] = lambda v: escape(str(v)) if v is not None else ''
    env.filters['nzdate'] = nzdate
    return env


def deep_merge(base, override):
    """Nested-dict merge; override wins. Lists and scalars are replaced whole."""
    merged = dict(base)
    for k, v in override.items():
        if isinstance(merged.get(k), dict) and isinstance(v, dict):
            merged[k] = deep_merge(merged[k], v)
        else:
            merged[k] = v
    return merged


def interpolate(node, env: Environment, context: dict):
    """Render every placeholder-bearing string in the data through Jinja."""
    if isinstance(node, str) and ('{{' in node or '{%' in node):
        return env.from_string(node).render(**context)
    if isinstance(node, dict):
        return {k: interpolate(v, env, context) for k, v in node.items()}
    if isinstance(node, list):
        return [interpolate(v, env, context) for v in node]
    return node


def load(edition_file: Path, env: Environment) -> dict:
    data = yaml.safe_load(edition_file.read_text())
    if COMMON_FILE.exists():
        data = deep_merge(yaml.safe_load(COMMON_FILE.read_text()), data)
    return interpolate(data, env, data)


def render(data: dict, env: Environment) -> dict[str, str]:
    """Render both encodings: the canonical ISO 19115-3 record, and an ISO
    19139 (gmd:) companion for catalogues that cannot parse 19115-3 — the
    Koordinates platform behind the LRIS Portal imports title, description
    and tags from 19139 only."""
    s = data['scalars']
    slug = s['layer_slug']
    derived = {
        # No layer_slug means no LRIS layer (not yet published, or an edition
        # that will never be): the templates omit every LRIS-layer block.
        'layer_url': f"https://lris.scinfo.org.nz/layer/{slug}/" if slug else None,
        'qml_download_url': (
            'https://lris.scinfo.org.nz/services/api/v1.x/documents/'
            f"{s['qml_document_id']}/versions/{s['qml_document_version']}/download/"
        ),
        'qml_page_url': f"https://lris.scinfo.org.nz/document/{s['qml_slug']}/",
    }
    return {suffix: env.get_template(template).render(**data, **derived)
            for suffix, template in [('iso19115-3', 'iso19115-3.xml.j2'),
                                     ('iso19139', 'iso19139.xml.j2')]}


def check_data(data: dict) -> list[str]:
    """Data-level checks, independent of encoding. Run once per edition."""
    problems: list[str] = []
    # every class must have a value, a label and a colour
    for c in data['classes']:
        if c.get('colour') is None:
            problems.append(f"class {c['value']} ({c['label']}) has no colour")
        elif not re.fullmatch(r'#[0-9a-fA-F]{6}', c['colour']):
            problems.append(f"class {c['value']} colour is not a 6-digit hex: {c['colour']}")
        elif c['colour'].lower() not in (c['definition'] or '').lower():
            problems.append(f"class {c['value']} colour missing from its definition text")
    values = [c['value'] for c in data['classes']]
    if len(set(values)) != len(values):
        problems.append('duplicate class values')
    if values != sorted(values):
        problems.append('class values are not in ascending order')
    return problems


def check_xml(xml_text: str) -> list[str]:
    """Structural self-checks on one rendered encoding. Not a substitute for
    schema validation."""
    problems: list[str] = []
    try:
        root = etree.fromstring(xml_text.encode())
    except etree.XMLSyntaxError as e:
        return [f'not well-formed: {e}']

    ns = {k: v for k, v in root.nsmap.items() if k}
    if etree.QName(root).namespace == 'http://www.isotc211.org/2005/gmd':
        order = GMD_ORDER
        step_path = './/gmd:LI_ProcessStep/gmd:description/gco:CharacterString'
        statement_path = './/gmd:LI_Lineage/gmd:statement'
        portrayal_path = 'gmd:portrayalCatalogueInfo'
    else:
        order = MDB_ORDER
        step_path = './/mrl:LI_ProcessStep/mrl:description/gco:CharacterString'
        statement_path = './/mrl:LI_Lineage/mrl:statement'
        portrayal_path = 'mdb:portrayalCatalogueInfo'

    children = [etree.QName(c).localname for c in root if isinstance(c.tag, str)]
    rank = {n: i for i, n in enumerate(order)}
    for a, b in zip(children, children[1:]):
        if a not in rank:
            problems.append(f'unknown MD_Metadata child: {a}')
        elif b in rank and rank[b] < rank[a]:
            problems.append(f'element order: {b} must precede {a}')

    # lineage must be present and ordered
    steps = root.findall(step_path, ns)
    if not root.findall(statement_path, ns):
        problems.append('lineage statement missing')
    for i, st in enumerate(steps, 1):
        if not st.text.startswith(f'Step {i} of {len(steps)} '):
            problems.append(f'process step {i} is misnumbered')

    # a record citing no portrayal catalogue has lost the palette provenance
    if not root.findall(portrayal_path, ns):
        problems.append('no portrayalCatalogueInfo: palette provenance is unrecorded')
    return problems


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('edition', nargs='?', help='YAML file in editions/, e.g. 2024-25.yaml')
    ap.add_argument('--all', action='store_true', help='render every edition')
    ap.add_argument('--check', action='store_true', help='report only; do not write')
    args = ap.parse_args()

    if args.all:
        targets = sorted(EDITION_DIR.glob('*.yaml'))
    elif args.edition:
        targets = [EDITION_DIR / Path(args.edition).name]
    else:
        ap.error('give an edition file or --all')

    env = make_env()
    failed = False
    for target in targets:
        if not target.exists():
            print(f'! {target} not found', file=sys.stderr)
            failed = True
            continue
        data = load(target, env)
        renders = render(data, env)
        slug = (data['scalars']['layer_slug']
                or 'nz-cost-effective-land-cover-' + data['edition'].replace('/', '-'))

        problems = check_data(data)
        problems += [f'[{suffix}] {p}'
                     for suffix, xml_text in renders.items()
                     for p in check_xml(xml_text)]
        if problems:
            failed = True
            print(f'\n{target.name}: {len(problems)} problem(s)')
            for p in problems:
                print(f'  ! {p}')
        else:
            print(f'\n{target.name}: checks passed')

        # CHECK markers come from the data, so scanning one encoding suffices;
        # dedupe in case a marker still renders slightly differently.
        xml_text = renders['iso19115-3']
        comments = re.findall(r'<!--(.*?)-->', xml_text, re.DOTALL)
        checks = [' '.join(m.split()) for c in comments
                  for m in re.findall(r'CHECK: (.*)', c, re.DOTALL)]
        inline = re.findall(r'CHECK: ([^<]+)', re.sub(r'<!--.*?-->', '', xml_text, flags=re.DOTALL))
        checks = list(dict.fromkeys(checks))
        if checks or inline:
            print(f'  {len(checks) + len(inline)} unresolved CHECK item(s):')
            for c in checks:
                print(f'    - {c}')
            for c in inline:
                print(f'    - (in element text) {" ".join(c.split())}')

        if not args.check:
            BUILD_DIR.mkdir(exist_ok=True)
            for suffix, xml_text in renders.items():
                out = BUILD_DIR / f"{slug}_{suffix}.xml"
                out.write_text(xml_text)
                print(f'  wrote {out.relative_to(ROOT)} '
                      f"({len(data['classes'])} classes, {len(data['lineage_steps'])} process steps)")

    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
