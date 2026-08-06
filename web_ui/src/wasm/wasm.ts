

export type SZIterator = number;
export type SZCard = number;

export default interface SZ {
    _iterator_new(obj_enum: number): SZIterator;
    _iterator_remaining(it: SZIterator): number;
    _iterator_size(it: SZIterator): number;

    _iterator_seek_card(it: SZIterator, idx: number): SZCard;
    _iterator_next_card(it: SZIterator): SZCard;
    _iterator_prev_card(it: SZIterator): SZCard;


    _malloc(size: number): number;
    _free(ptr: number): void;
};

import createModule from "./sz.js"
export const module = await createModule() as SZ;
