# NZ Cost-Effective Land Cover — ISO 19115 metadata

Generates one ISO 19115-1:2014 metadata record per edition of the land-cover series,
in the ISO 19115-3:2018 XML encoding.

```
template/iso19115-3.xml.j2   ISO structure. Rarely changes.
common.yaml                  Method- and series-level facts — most of the record.
editions/<year>.yaml         Per-edition scalars and cross-references. Small.
render.py                    YAML + template -> XML, with structural self-checks.
build/                       Generated records. Do not edit by hand.
gdalinfo.txt                 gdalinfo of the production raster; source of the grid
                             dimensions and exact bounding box in the records.
```

Nearly all prose lives in `common.yaml`, because the method is validated once (on the
2023/24 edition) and re-run on more recent imagery: the classes, lineage, quality
reports and abstract are method facts, not edition facts. An edition file contributes
identifiers, dates, the temporal window, and cross-references; edition-bound values in
the shared prose re-bind through placeholders.

## Environment

```bash
python3 -m venv .venv && .venv/bin/pip install -r requirements.txt
# or, with conda/mamba:
conda env create -f environment.yml && conda activate landcover-metadata
```

Each edition file is deep-merged over `common.yaml` (the edition wins on any key it
defines), and every string in the merged data is then rendered through Jinja with the
merged data as context. Prose therefore carries placeholders — `{{ edition }}`,
`{{ scalars.temporal_begin | nzdate }}` (the `nzdate` filter turns `2023-11-01` into
`1 November 2023`), `{{ scalars.ndvi_period }}`,
`{{ quality_reports[0].quantitative_result_percent }}` — so an edition-bound date or
figure is written once, as data, and every sentence that mentions it stays correct.
Placeholders resolve in one pass: point them at literal values, never at other
placeholder-bearing prose.

## Adding the next edition

```bash
cp editions/2024-25.yaml editions/2025-26.yaml
$EDITOR editions/2025-26.yaml   # new uuid (uuidgen), dates, temporal + NDVI windows,
                                # layer id/slug, back-reference to the previous edition
python3 render.py 2025-26.yaml
```

Then add a back-reference from the previous edition, so the pair is linked in both
directions: append to its `associated_editions` and re-render it.

`render.py --all` rebuilds every edition. `--check` reports without writing.

The shared prose re-binds to the new scalars via the placeholders, so season dates and
the NDVI window need only be set once, in `scalars`. If the method itself changes for
the new edition (a different LCDB version burned in, retrained models), do NOT edit the
affected keys in `common.yaml` — that would rewrite the already-published editions on
their next re-render. Copy the affected keys into the new edition's file and edit them
there; the edition file overrides `common.yaml` key by key.

## What the checks catch

`render.py` runs structural checks on each render and exits non-zero on failure:

- **Element order.** ISO 19115-1 fixes the order of `MD_Metadata` children and a
  validator will reject any other order. Schema validation needs the XSDs from
  `schemas.isotc211.org`, so the order is asserted against a table in `render.py`
  instead. This is a substitute for schema validation, not a replacement — run the
  output through GeoNetwork or an XSD validator before publishing.
- **Class integrity.** Every class needs a value, a label and a 6-digit hex colour,
  the colour must actually appear in the definition text, values must be unique and
  ascending.
- **Lineage.** A statement must be present, and process steps must be numbered
  consecutively — `render.py` inserts `Step n of m` from list order, so reordering or
  inserting a step in the YAML renumbers the whole sequence automatically.
- **Portrayal.** The record must cite a portrayal catalogue, or the palette's
  provenance is unrecorded.
- **Unresolved `CHECK:` items** are listed on every render, both those in XML comments
  and those embedded in element text.

Templating uses `StrictUndefined`: a mistyped required key fails loudly rather than
rendering an empty element. Optional keys are accessed with `.get()` in the template.

## Design decisions worth knowing before you change things

**One record per edition; nothing is factored into a shared parent.** ISO permits a
series parent with `parentMetadata`, and the template supports it
(`scalars.parent_metadata_uuid`), but harvesters and GeoNetwork largely treat records
as standalone. A child record omitting its own class table becomes undiscoverable on
class terms. Duplication is deliberate.

**`common.yaml` factors the YAML source, not the records.** The rendered XML stays
fully self-contained per edition — each record still carries its own class table,
lineage and quality reports for harvesters. A change to `common.yaml` alters *every*
edition on its next re-render, including published ones; the override discipline above
is what protects published records. The validation is deliberately worded as a fact
about the 2023/24 edition ("validated once, on the unfiltered 2023/24 classification")
so it remains true verbatim in every later edition's record.

**Editions are cross-references, not revisions.** `associated_editions` defaults to
`association_type: crossReference` rather than `revisionOf`, because a new annual
snapshot does not supersede the previous year — the earlier edition remains valid for
its own period. Use `revisionOf` only for a genuine correction to a published edition.

**Colours live in two places on purpose.** ISO 19115-1's `portrayalCatalogueInfo`
carries only a *citation* to an external portrayal catalogue (ISO 19117), not inline
symbology. So the QML is cited formally, and each hex value is repeated in its class
definition so the palette travels with the class list. The QML remains authoritative;
`render.py` checks the two stay consistent.

**The QML download URL is version-pinned.** It fixes the style to the edition it
describes, which is what a portrayal citation should do, but it must be updated if a
revised style is uploaded as a new document version. The unpinned document page is
carried alongside as a fallback.

**`dimensionSize` is a pixel count, not an extent.** `scalars.columns` / `scalars.rows`
are the raster's width and height in cells; leaving them null emits
`nilReason="unknown"`, which is valid. The geographic extent is separate, and ISO
requires it in decimal degrees. To also record the native NZTM extent, populate
`scalars.bbox_nztm` (`min_e`, `min_n`, `max_e`, `max_n`) and the template emits an
`EX_BoundingPolygon` with `srsName=EPSG::2193` alongside the lat/long bounding box.

## Per-edition facts most likely to change

Beyond the obvious dates, layer ID and temporal extent:

- **The LCDB version burned in** for wetlands, orchards/vineyards and glacial lakes.
  v6.0 now exists. Changing it touches three class definitions, the LCDB lineage
  source, the final process step, the abstract, and the inherited-date use limitation.
- **Validation figures** in `quality_reports` and in the abstract.
- **Training imagery and model provenance** if the U-Net models were retrained.
- **Reserved class value 5** (Cloud), currently unused.
