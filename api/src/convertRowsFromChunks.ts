import { DuckDBDataChunk } from './DuckDBDataChunk';
import { DuckDBValueConverter } from './DuckDBValueConverter';

export function convertRowsFromChunks<T>(
  chunks: readonly DuckDBDataChunk[],
  converter: DuckDBValueConverter<T>
): T[][] {
  const rows: T[][] = [];
  for (const chunk of chunks) {
    const rowCount = chunk.rowCount;
    for (let rowIndex = 0; rowIndex < rowCount; rowIndex++) {
      rows.push(chunk.convertRowValues(rowIndex, converter));
    }
  }
  return rows;
}
