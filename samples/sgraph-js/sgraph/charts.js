class ChartNone {
    static partition_name = "chart.none";
}

class UnilevelChart {
    static partition_name = "chart.unilevel";

    constructor(reader) {
        this.seq = new Sequence(ParameterDecl, reader);
        this.requirement = new ExprIndex(reader);
    }
}

class MultilevelChart {
    static partition_name = "chart.multilevel";

    constructor(reader) {
        this.seq = new HeapSequence(HeapSort.Values.Chart, reader);
    }
}

function symbolic_for_chart_sort(sort) {
    switch (sort) {
    case ChartIndex.Sort.None:
        return ChartNone;
    case ChartIndex.Sort.Unilevel:
        return UnilevelChart;
    case ChartIndex.Sort.Multilevel:
        return MultilevelChart;
    default:
        console.error(`Bad sort: ${sort}`);
        return null;
    }
}