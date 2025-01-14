import duckdb from '@duckdb/node-bindings';
import { DuckDBType } from './DuckDBType';
import { DuckDBVector } from './DuckDBVector';
import { DuckDBValue } from './values';

export class DuckDBDataChunk {
  public readonly chunk: duckdb.DataChunk;
  private readonly vectors: DuckDBVector[] = [];
  constructor(chunk: duckdb.DataChunk) {
    this.chunk = chunk;
  }
  public static create(types: DuckDBType[]): DuckDBDataChunk {
    return new DuckDBDataChunk(duckdb.create_data_chunk(types.map(t => t.toLogicalType().logical_type)));
  }
  public reset() {
    duckdb.data_chunk_reset(this.chunk);
  }
  public get columnCount(): number {
    return duckdb.data_chunk_get_column_count(this.chunk);
  }
  public getColumnVector(columnIndex: number): DuckDBVector {
    if (this.vectors[columnIndex]) {
      return this.vectors[columnIndex];
    }
    const vector = DuckDBVector.create(
      duckdb.data_chunk_get_vector(this.chunk, columnIndex),
      this.rowCount
    );
    this.vectors[columnIndex] = vector;
    return vector;
  }
  public getColumnValues(columnIndex: number): DuckDBValue[] {
    return this.getColumnVector(columnIndex).toArray();
  }
  public getColumns(): DuckDBValue[][] {
    const columns: DuckDBValue[][] = [];
    const columnCount = this.columnCount;
    for (let columnIndex = 0; columnIndex < columnCount; columnIndex++) {
      columns.push(this.getColumnValues(columnIndex));
    }
    return columns;
  }
  public getRows(): DuckDBValue[][] {
    const rows: DuckDBValue[][] = [];
    const vectors: DuckDBVector[] = [];
    const columnCount = this.columnCount;
    for (let columnIndex = 0; columnIndex < columnCount; columnIndex++) {
      vectors.push(this.getColumnVector(columnIndex));
    }
    const rowCount = this.rowCount;
    for (let rowIndex = 0; rowIndex < rowCount; rowIndex++) {
      const row: DuckDBValue[] = [];
      for (let columnIndex = 0; columnIndex < columnCount; columnIndex++) {
        row.push(vectors[columnIndex].getItem(rowIndex));
      }
      rows.push(row);
    }
    return rows;
  }
  public get rowCount(): number {
    return duckdb.data_chunk_get_size(this.chunk);
  }
  public set rowCount(count: number) {
    duckdb.data_chunk_set_size(this.chunk, count);
  }
}
