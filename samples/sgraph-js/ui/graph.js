// D3 + HTML canvas: https://www.freecodecamp.org/news/d3-and-canvas-in-3-steps-8505c8b27444

function transposed() {
    return options != null && options.transpose;
}

function height() {
    if (transposed()) {
        return graph.drawing.native_width;
    }
    return graph.drawing.native_height;
}

function width() {
    if (transposed())
        return graph.drawing.native_height;
    return graph.drawing.native_width;
}

var x = d3.scaleLinear()
          .range([0.0, graph.drawing.native_width])
          .domain([0.0, 1.0]); // TODO

var y = d3.scaleLinear()
          .range([0.0, graph.drawing.native_height])
          .domain([0.0, 1.0]); // TODO

function zoom_filter(event) {
    // for some reason 'event' is always undefined despite the documentation saying
    // it should be an event object: https://github.com/d3/d3-zoom/blob/v2.0.0/README.md#zoom_filter
    return true;
}

var zoom = d3.zoom()
             .filter(zoom_filter)
             // Delay the 'zoomed' callback so we don't modify a large data model hundreds of times
             // in a second.
             .on("zoom", debounce(zoomed));

var graph_canvas = d3.select("#icicle")
                     .append("canvas")
                     .classed("graph-canvas", true)
                     .attr("width", graph.drawing.native_width)
                     .attr("height", graph.drawing.native_height)
                     .call(zoom)
                     .on("dblclick.zoom", null);

var render_canvas = d3.select(document.createElement("canvas"))
                      .attr("width", graph.drawing.native_width)
                      .attr("height", graph.drawing.native_height);

var backing_canvas = d3.select("#icicle")
                       .append("canvas")
                       .classed("backing-graph-canvas", true)
                       .attr("width", graph.drawing.native_width)
                       .attr("height", graph.drawing.native_height)
                       .attr("hidden", true);

class PartitionDataHelper
{
    constructor(root) {
        this.root = root;
    }

    partition() {
        var p = d3.partition()
                  .size([width(), height()])
                  .padding(0)
                  // Do not round results so that the graph will not truncate large sets of decls.
                  .round(false);
        p(this.root);
    }
}

// Breadcrumb dimensions: width, height, spacing, width of tip/tail.
var b = {
    w: 150, h: 30, s: 3, t: 10
};

var total_size = 0;

const delim = '`_';
function is_meta_name(name) {
    return name.includes(delim);
}

function append_name_meta(name, index) {
    return name + `${delim}${index.sort},${index.index}`;
}

function name_from_meta(str) {
    var parts = str.split(delim);
    return parts[0];
}

function name_and_index_from_meta(str) {
    var parts = str.split(delim);
    var name = parts[0];
    var str_index = parts[1];
    var index_parts = str_index.split(',');
    return { name: name, index: { sort: parseInt(index_parts[0]), index: parseInt(index_parts[1]) } };
}

function color_for_index(index) {
    return options.color_for_index(index);
}

function color_from_meta(str) {
    if (!is_meta_name(str)) {
        return d3.color('lightgray');
    }
    const meta = name_and_index_from_meta(str);
    return d3.color(color_for_index(meta.index));
}

function secondary_color_from_meta(str) {
    if (!is_meta_name(str))
        return 'none';
    const meta = name_and_index_from_meta(str);
    if (meta.index.sort != DeclIndex.Sort.Template)
        return 'none';
    const template_decl = sgraph.resolver.read(TemplateDecl, meta.index.index);
    return color_for_index(template_decl.entity.decl);
}

function adjust_color_if_necessary(color, selected) {
    if (!color)
        return color;
    if (selected)
        return color;
    // Note: I tried interpolate here and it seemed to have more of an impact on perf over
    // the opacity method.  The opacity method has the issue of overlapping rects creating
    // the illusion of a selection.  Revisit later.
    //var new_color = d3.interpolate(d3.color(color), d3.color("white"))(0.75);
    var new_color = d3.color(color);
    new_color.opacity = options.deselected_opacity;
    return new_color;
}

function draw_rect(context, rect, fill, gradient_color, selected) {
    fill = adjust_color_if_necessary(fill, selected);
    if (gradient_color == null || gradient_color == 'none') {
        context.fillStyle = fill;
        context.fillRect(rect.left, rect.top, rect.width, rect.height);
        return;
    }
    const gradient_fill = adjust_color_if_necessary(d3.color(gradient_color), selected);
    const gradient = context.createLinearGradient(rect.left, rect.top, rect.left, rect.top + rect.height);
    gradient.addColorStop(0, fill);
    gradient.addColorStop(0.5, gradient_fill);
    gradient.addColorStop(1, gradient_fill);
    context.fillStyle = gradient;
    context.fillRect(rect.left, rect.top, rect.width, rect.height);
}

function draw_rect_info(rect) {
    const threshold = 2;
    // When the graph is transposed we actually want to check the node
    // height value because nodes will be stacked, not paired side-by-side.
    if (transposed()) {
        return rect.height > threshold;
    }
    return rect.width > threshold;
}

function draw_node(context, node, rect, selected) {
    const node_fill = node.getAttribute("fill");
    const gradient_color = node.getAttribute("gradient-fill");
    draw_rect(context, rect, node_fill, gradient_color, selected);
    if (draw_rect_info(rect)) {
        context.strokeStyle = adjust_color_if_necessary("#000000", selected);
        context.strokeRect(rect.left, rect.top, rect.width, rect.height);
        context.fillStyle = adjust_color_if_necessary("#000000", selected);
        const text = node.getAttribute("node-name");
        context.fillText(text, rect.left + 2, rect.top + 14, rect.width);
    }
}

function draw_backing_rect(context, node, rect) {
    context.fillStyle = node.getAttribute("backing-fill");
    context.fillRect(rect.left, rect.top, rect.width, rect.height);
}

function simple_draw_rect(context, node, rect) {
    context.fillStyle = node.getAttribute("fill");
    context.fillRect(rect.left, rect.top, rect.width, rect.height);
}

var running_fps_average = 0;
var fps_count = 0;
function draw_fps(context, fps) {
    context.fillStyle = "#000000";
    context.fillText(`FPS: ${fps}`, 50, 50);
    if (!isNaN(fps)) {
        running_fps_average += fps;
        fps_count += 1;
    }
    var fps_avg = running_fps_average / fps_count;
    context.fillText(`FPS (average): ${fps_avg}`, 50, 70);
}

function render_nodes(canvas) {
    var render_ctx = canvas.getContext("2d");
    render_ctx.clearRect(0, 0, graph.drawing.native_width, graph.drawing.native_height);
    render_ctx.font = "14px serif";
    var rect = { left: null, top: null, height: null, width: null };
    for (var node of graph.drawing.working_data) {
        rect.left = parseInt(node.getAttribute("x"));
        rect.top = parseInt(node.getAttribute("y"));
        // Take the max between the height/width and 1px so the viewer has some
        // visual indication that _something_ is in this region.
        rect.height = Math.max(parseInt(node.getAttribute("height")), 1);
        rect.width = Math.max(parseInt(node.getAttribute("width")), 1);

        const selected = node.getAttribute("selected") == "true";

        draw_node(render_ctx, node, rect, selected);
    }
    // If the nodes are ever rendered, the backing canvas is implicitly dirty.
    backing_canvas_dirty = true;
}

const text_box_height = 20;
// +2 padding pixels.
const text_box_padding = 2;

function draw_text_box_at(context, text, point) {
    const measured = context.measureText(text);
    var rect = { left: null, top: null, height: null, width: null };
    rect.width = measured.width + text_box_padding;
    rect.height = text_box_height;
    rect.left = point.x;
    rect.top = point.y;
    // We want to ensure the text box is visible.  If it exceeds the viewable canvas area
    // we will recenter it.
    // x first.
    if (rect.left + rect.width > graph.drawing.native_width) {
        // Anchor to the side.
        rect.left = graph.drawing.native_width - rect.width;
    }
    // y next.
    if (rect.top < 0) {
        // Anchor to the top.
        rect.top = 0;
    }
    context.fillStyle = "#ffffff";
    context.fillRect(rect.left, rect.top, rect.width, rect.height);
    context.strokeStyle = "#000000";
    context.strokeRect(rect.left, rect.top, rect.width, rect.height);
    context.fillStyle = "#000000";
    context.fillText(text, rect.left + 1, rect.top + 14, rect.width);
    return rect;
}

function draw_node_hover(context, hover) {
    const text = `${hover.name} : ${hover.index}`;
    var point = {
                // Add padding away from mouse cursor.
                x: hover.x + text_box_padding,
                // Float above the cursor.
                y: hover.y - text_box_height - text_box_padding };
    draw_text_box_at(context, text, point);
}

function draw(canvas, is_backing_canvas, fps) {
    var context = canvas.node().getContext("2d");

    context.clearRect(0, 0, graph.drawing.native_width, graph.drawing.native_height);

    context.font = "14px serif";

    if (is_backing_canvas) {
        var rect = { left: null, top: null, height: null, width: null };
        for (var node of graph.drawing.working_data) {
            rect.left = parseInt(node.getAttribute("x"));
            rect.top = parseInt(node.getAttribute("y"));
            // Take the max between the height/width and 1px so the viewer has some
            // visual indication that _something_ is in this region.
            rect.height = Math.max(parseInt(node.getAttribute("height")), 1);
            rect.width = Math.max(parseInt(node.getAttribute("width")), 1);

            const selected = node.getAttribute("selected") == "true";

            // Note: we only want to draw the backing rect if the node is selected.  This
            // will prevent mouse interactivity on the 'unselected' rects.
            if (selected)
                draw_backing_rect(context, node, rect);
        }
        return;
    }

    var render_canvas_element = render_canvas.node();
    if (canvas_dirty && !is_backing_canvas)
        render_nodes(render_canvas_element);
    // Note: we apply the render canvas to the native width/height because we've possibly downscaled it
    // based on DPI settings.
    context.drawImage(render_canvas_element, 0, 0, graph.drawing.native_width, graph.drawing.native_height);

    if (hovered_node != null)
        draw_node_hover(context, hovered_node);

    if (options != null && options.show_fps)
        draw_fps(context, fps);
}

class CanvasColorGenerator {
    constructor() {
        this.next_color = 1;
    }

    next() {
        var r = 0;
        var g = 0;
        var b = 0;
        if (this.next_color < 0xffffff) {
            r = this.next_color & 0xff;
            g = (this.next_color & 0xff00) >> 8;
            b = (this.next_color & 0xff0000) >> 16;
            this.next_color += 1;
        }
        return CanvasColorGenerator.key(r, g, b);
    }

    reset() {
        this.next_color = 1;
    }

    static key(r, g, b) {
        return `rgb(${r},${g},${b})`;
    }
}

class NodeToColorMapper {
    constructor() {
        this.map = new Map();
        this.generator = new CanvasColorGenerator();
    }

    map_node(n) {
        const key = this.generator.next();
        this.map.set(key, n);
        return key;
    }

    get(key) {
        return this.map.get(key);
    }

    reset() {
        this.generator.reset();
        this.map.clear();
    }
}

var canvas_dirty = false;
var backing_canvas_dirty = false;

var last_update_time = 0;

function map_object(o) {
    return { key: o[0], value: o[1] };
}

function node_x(d) {
    if (transposed())
        return d.y0;
    return d.x0;
}

function node_y(d) {
    if (transposed())
        return d.x0;
    return d.y0;
}

function node_width(d) {
    if (transposed())
        return d.y1 - d.y0;
    return d.x1 - d.x0;
}

function transform_node_width(d, t) {
    if (transposed())
        return t(d.y1) - t(d.y0);
    return t(d.x1) - t(d.x0);
}

function node_height(d) {
    if (transposed())
        return d.x1 - d.x0;
    return d.y1 - d.y0;
}

function transform_node_height(d, t) {
    if (transposed())
        return t(d.x1) - t(d.x0);
    return t(d.y1) - t(d.y0);
}

function setup_dpi(canvas) {
    // Reference: https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Optimizing_canvas#scaling_for_high_resolution_displays
    canvas.style.width = `${graph.drawing.native_width}px`;
    canvas.style.height = `${graph.drawing.native_height}px`;

    var dpr = window.devicePixelRatio || 1;
    canvas.width = graph.drawing.native_width * dpr;
    canvas.height = graph.drawing.native_height * dpr;
    var dpr = window.devicePixelRatio || 1;
    var context = canvas.getContext("2d");
    context.scale(dpr, dpr);
}

function build_graph_data(root) {
    root = d3.hierarchy(Object.entries(root).map(map_object)[0], function(d) {
            return Object.entries(d.value).map(map_object);
        })
        .sum(function(d) { return d.value })
        .sort(function(a, b) { return b.value - a.value; });

    graph.drawing.partition = new PartitionDataHelper(root);
    graph.drawing.node_mapper = new NodeToColorMapper();

    graph.drawing.partition.partition();
    var percentage = 100;
    var percentageString = percentage + "%";

    d3.select("#percentage")
      .text(percentageString);

    d3.select("#explanation")
      .style("visibility", "");

    var sequenceArray = root.ancestors().reverse();
    update_breadcrumbs(sequenceArray, percentageString);

    graph.drawing.custom_base_element = document.createElement("custom");
    graph.drawing.working_data = d3.select(graph.drawing.custom_base_element);

    graph.drawing.working_data = graph.drawing.working_data.selectAll("custom.rect")
                             .data(root.descendants())
                             .enter()
                             .append("custom")
                             .attr("class", "rect")
                             .attr("x", function(d) { return node_x(d); })
                             .attr("y", function(d) { return node_y(d); })
                             .attr("width", function(d) { return node_width(d); })
                             .attr("height", function(d) { return node_height(d); })
                             .attr("fill", function(d) { return color_from_meta(d.data.key); })
                             .attr("gradient-fill", function(d) { return secondary_color_from_meta(d.data.key); })
                             .attr("node-name", function(d) { return name_from_meta(d.data.key); })
                             .attr("backing-fill", function(d) { return graph.drawing.node_mapper.map_node(d); })
                             .attr("selected", function(d) { return true; });

    //get total size from rect
    total_size = graph.drawing.working_data.node().__data__.value;
}

function init_graph(root) {
    graph.data.original_data = root;
    setup_dpi(graph_canvas.node());
    setup_dpi(render_canvas.node());
    init_breadcrumb_trail();
    // Note: we do not scale the backing-canvas because its size needs to correspond directly to unscaled
    // mouse coordinates on screen.
    build_graph_data(root);

    requestAnimationFrame(animation_frame);
    function animation_frame(elapsed) {
        // Aim for 60fps.
        if (elapsed - last_update_time > 16) {
            var fps = 1000 / (elapsed - last_update_time);
            draw(graph_canvas, false, fps);
            last_update_time = elapsed;
        }
        requestAnimationFrame(animation_frame);
    }

    // Center the view on the top-most node.
    reset_view();
}

function zoomed(e) {
    var new_x = e.transform.rescaleX(x);
    var new_y = e.transform.rescaleY(y);

    if (options.graph_animations)
    {
        canvas_dirty = true;
        graph.drawing.working_data.transition()
                   .duration(750)
                   .attr("x", function(d) { return new_x(node_x(d)); })
                   .attr("y", function(d) { return new_y(node_y(d)); })
                   .attr("width", function(d) { return transform_node_width(d, new_x); })
                   .attr("height", function(d) { return transform_node_height(d, new_y); })
                   .end()
                        .then(() => {
                            canvas_dirty = false;
                            // Make sure that we show the nodes in their final position.
                            render_nodes(render_canvas.node()); } )
                        .catch(() => {});
    }
    else
    {
        graph.drawing.working_data
            .attr("x", function(d) { return new_x(node_x(d)); })
            .attr("y", function(d) { return new_y(node_y(d)); })
            .attr("width", function(d) { return transform_node_width(d, new_x); })
            .attr("height", function(d) { return transform_node_height(d, new_y); });
        render_nodes(render_canvas.node());
    }
}

function screen_space_to_point(event) {
    // https://stackoverflow.com/a/12114213
    // The ordering of layer vs offset is important.  We want to prefer
    // offset if we can as this is the position of the mouse in the
    // canvas bounding box.
    const mouse_x = event.offsetX || event.layerX;
    const mouse_y = event.offsetY || event.layerY;
    return { x: mouse_x, y: mouse_y };
}

function extract_node_from_backing_canvas(point) {
    const backing_canvas_context = backing_canvas.node().getContext("2d");
    const color = backing_canvas_context.getImageData(point.x, point.y, 1, 1).data;
    const key = CanvasColorGenerator.key(color[0], color[1], color[2]);

    const node = graph.drawing.node_mapper.get(key);
    return node;
}

function rebuild_backing_canvas_if_necessary() {
    if (backing_canvas_dirty) {
        console.log("backing canvas refreshed");
        draw(backing_canvas, true);
        backing_canvas_dirty = false;
    }
}

d3.select(".graph-canvas")
  .on("click", function(e) {
    // Render the backing canvas to get the correct color.
    rebuild_backing_canvas_if_necessary();
    const point = screen_space_to_point(e);
    const node = extract_node_from_backing_canvas(point);
    if (node == null)
        return;
    navigate_to(node);
});

var hovered_node = null;
d3.select(".graph-canvas")
    .on("mousemove", function(e) {
        rebuild_backing_canvas_if_necessary();
        const point = screen_space_to_point(e);
        const node = extract_node_from_backing_canvas(point);
        if (node == null) {
            hovered_node = null;
            return;
        }
        // If we're pointing at the same node as before, we don't need to update the node, just the position.
        if (hovered_node != null && hovered_node.node == node)
        {
            hovered_node.x = point.x;
            hovered_node.y = point.y;
            return;
        }

        var index = "N/A";
        var name = "N/A";
        if (node.data.name != "root" && is_meta_name(node.data.key)) {
            var meta = name_and_index_from_meta(node.data.key);
            index = `DeclIndex{${sort_to_string(DeclIndex, meta.index.sort)},${meta.index.index}}`;
            name = `${meta.name}`;
        }
        console.log("new hover node");
        hovered_node = { node: node, index: index, name: name, x: point.x, y: point.y };
});

// A helper to skip the 'decl_selected' callback during 'navigate_to'.
const SkipSelection = {};

function navigate_to(d, skip_selection) {
    if (transposed()) {
        x.domain([d.y0, height()]).range([d.depth ? 20 : 0, height()]);
        y.domain([d.x0, d.x1]);
    }
    else {
        x.domain([d.x0, d.x1]);
        y.domain([d.y0, height()]).range([d.depth ? 20 : 0, height()]);
    }

    if (options.graph_animations)
    {
        canvas_dirty = true;
        graph.drawing.working_data.transition()
                   .duration(750)
                   .attr("x", function(d) { return x(node_x(d)); })
                   .attr("y", function(d) { return y(node_y(d)); })
                   .attr("width", function(d) { return transform_node_width(d, x); })
                   .attr("height", function(d) { return transform_node_height(d, y); })
                   .end()
                        .then(() => {
                            canvas_dirty = false;
                            // Make sure that we show the nodes in their final position.
                            render_nodes(render_canvas.node()); } )
                        .catch(() => {})
                        .finally(() => 
                            // Adjust the zoom transform so that zooming will work smoothly around the newly
                            // centered node.  Solution from: https://github.com/d3/d3-zoom/issues/107.
                            // Note: do this after the transition to the clicked node so that the even that fires
                            // after the transition is modified will do minimal work.
                            zoom.transform(graph_canvas, d3.zoomIdentity));
    }
    else
    {
        graph.drawing.working_data
            .attr("x", function(d) { return x(node_x(d)); })
            .attr("y", function(d) { return y(node_y(d)); })
            .attr("width", function(d) { return transform_node_width(d, x); })
            .attr("height", function(d) { return transform_node_height(d, y); });
        render_nodes(render_canvas.node());
        // Adjust the zoom transform so that zooming will work smoothly around the newly
        // centered node.  Solution from: https://github.com/d3/d3-zoom/issues/107.
        zoom.transform(graph_canvas, d3.zoomIdentity);
    }

    // code to update the BreadcrumbTrail();
    var percentage = (100 * d.value / total_size).toPrecision(3);
    var percentageString = percentage + "%";
    if (percentage < 0.1) {
        percentageString = "< 0.1%";
    }

    d3.select("#percentage")
        .text(percentageString);

    d3.select("#explanation")
        .style("visibility", "");

    const sequenceArray = d.ancestors().reverse();
    update_breadcrumbs(sequenceArray, percentageString);

    if (skip_selection != undefined && skip_selection == SkipSelection)
        return;
    // We only want to fire the handler when a node is actually clicked (in this handler).
    decl_selected(sequenceArray);
}

function clicked(d) {
    navigate_to(d);
}

function init_breadcrumb_trail() {
    // Add the svg area.
    var trail = d3.select("#breadcrumb")
                  .append("svg")
                  .attr("width", graph.drawing.native_width)
                  .attr("height", 50)
                  .attr("id", "trail");
    // Add the label at the end, for the percentage.
    trail.append("text")
         .attr("id", "endlabel")
         .style("fill", "#000");

    // Make the breadcrumb trail visible, if it's hidden.
    d3.select("#trail")
      .style("visibility", "");
}

// Generate a string that describes the points of a breadcrumb polygon.
function breadcrumb_points(d, i) {
    var points = new Array();
    points.push("0,0");
    points.push(b.w + ",0");
    points.push(b.w + b.t + "," + (b.h / 2));
    points.push(b.w + "," + b.h);
    points.push("0," + b.h);
    if (i > 0) { // Leftmost breadcrumb; don't include 6th vertex.
        points.push(b.t + "," + (b.h / 2));
    }
    return points.join(" ");
}

// Update the breadcrumb trail to show the current sequence and percentage.
function update_breadcrumbs(nodeArray, percentageString) {
    // Data join; key function combines name and depth (= position in sequence).
    var trail = d3.select("#trail")
                  .selectAll("g")
                  .data(nodeArray, function(d) { return name_from_meta(d.data.key) + d.depth; });

    // Remove exiting nodes.
    trail.exit().remove();

    // Add breadcrumb and label for entering nodes.
    var entering = trail.enter().append("g");

    entering.append("polygon")
            .attr("points", breadcrumb_points)
            .style("fill", function(d) { return color_from_meta(d.data.key); });

    entering.append("text")
            .attr("x", (b.w + b.t) / 2)
            .attr("y", b.h / 2)
            .attr("dy", "0.35em")
            .attr("text-anchor", "middle")
            .text(function(d) { return name_from_meta(d.data.key); });

    // Merge enter and update selections; set position for all nodes.
    entering.merge(trail).attr("transform", function(d, i) {
        return "translate(" + i * (b.w + b.s) + ", 0)";
    });

    // Now move and update the percentage at the end.
    d3.select("#trail")
      .select("#endlabel")
      .attr("x", (nodeArray.length + 0.5) * (b.w + b.s))
      .attr("y", b.h / 2)
      .attr("dy", "0.35em")
      .attr("text-anchor", "middle")
      .text(percentageString);
}

function decl_selected(nodes) {
    if (nodes == null)
        return;
    const selected_node = nodes[nodes.length - 1];
    if (selected_node == null)
        return;
    on_decl_selected(selected_node.data.key);
}

// See filter.js.
function filtered_by_property(property_filter, index) {
    const symbolic = symbolic_for_decl_sort(index.sort);
    const symbolic_decl = sgraph.resolver.read(symbolic, index.index);
    if (!has_property(symbolic_decl, "basic_spec"))
        return false;
    if (implies(symbolic_decl.basic_spec.value, BasicSpecifiers.Values.C)
        && implies(property_filter, PropertyFilters.OnlyCLinkage))
        return true;
    if (implies(symbolic_decl.basic_spec.value, BasicSpecifiers.Values.NonExported))
        return implies(property_filter, PropertyFilters.NonExported);
    return implies(property_filter, PropertyFilters.Exported);
}

function meta_name_filtered(meta_name, filter) {
    // Filter out all meta 'names' which are not actual meta names.
    if (!is_meta_name(meta_name))
        return true;
    meta = name_and_index_from_meta(meta_name);
    if (filter.filter_sort()) {
        if (meta.index.sort != filter.sort)
            return true;
    }

    if (filter.filter_name()) {
        var name = meta.name;
        if (filter.name != name)
            return true;
    }

    if (filter.filter_props() != PropertyFilters.None)
        return filtered_by_property(filter.filter_props(), meta.index);
    return false;
}

function new_node_for(node) {
    var new_node = { name: node[0], value: { } };
    return new_node;
}

function merge_node_properties(node, props) {
    for (var p of props) {
        node.value[p.name] = p.value;
    }
}

// Walk the graph and prune subtrees, returning a new graph.
function walk_and_filter(node, filter) {
    const children = Object.entries(node[1]);
    // Leaf node.
    if (children.length == 0) {
        if (meta_name_filtered(node[0], filter)) {
            return null;
        }
        var new_node = new_node_for(node);
        new_node.value = 1;
        return new_node;
    }
    const new_children = children
                        .map(e => walk_and_filter(e, filter))
                        .filter(e => e != null);
    var new_node = new_node_for(node);
    if (new_children.length == 0) {
        if (meta_name_filtered(node[0], filter))
            return null;
        new_node.value = 1;
        return new_node;
    }
    merge_node_properties(new_node, new_children);
    return new_node;
}

function prune_with_graph_filter(filter) {
    // This algorithm will more-or-less perform a copy-on-write of nodes within the current graph.  If a node or its children
    // do not meet the filter criteria, it will be pruned.
    var new_root = { global: { } };
    const new_root_children = Object
                                .entries(graph
                                            .data
                                            .original_data
                                            .global)
                                .map(e => walk_and_filter(e, filter))
                                .filter(e => e != null);
    for (var p of new_root_children) {
        new_root.global[p.name] = p.value;
    }
    build_graph_data(new_root);
    reset_view();
}

function rebuild_original_graph() {
    build_graph_data(graph.data.original_data);
    reset_view();
}

function apply_graph_filter(filter) {
    if (filter.empty()) {
        build_graph_data(graph.data.original_data);
        graph.drawing.working_data.attr("selected", function(d) {
            return true;
        });
        reset_view();
        return;
    }
    if (filter.narrow_context) {
        prune_with_graph_filter(filter);
    }
    graph.drawing.working_data.attr("selected", function(d) {
        return !meta_name_filtered(d.data.key, filter);
    });
    // Redraw the nodes.
    render_nodes(render_canvas.node());
}

function reset_view() {
    navigate_to(graph.drawing.working_data.node().__data__);
}

// See options.js
function color_updated() {
    graph.drawing.working_data.attr("fill", function(d) { return color_from_meta(d.data.key); })
               .attr("gradient-fill", function(d) { return secondary_color_from_meta(d.data.key); });
    // Redraw the nodes.
    render_nodes(render_canvas.node());
}

function transpose_updated() {
    // According to https://github.com/d3/d3-selection/blob/v2.0.0/README.md#selection_data the data bound to the
    // D3 graph is cached by reference (not copied) so all we need to do if a transpose is requested is re-partition
    // the data blob and ask the data element to perform layout over the values.
    graph.drawing.partition.partition();
    if (options.graph_animations)
    {
        canvas_dirty = true;
        graph.drawing.working_data.transition()
                   .duration(750)
                   .attr("x", function(d) { return node_x(d); })
                   .attr("y", function(d) { return node_y(d); })
                   .attr("width", function(d) { return node_width(d); })
                   .attr("height", function(d) { return node_height(d); })
                   .end()
                        .then(() => {
                            canvas_dirty = false;
                            // Make sure that we show the nodes in their final position.
                            render_nodes(render_canvas.node()); } )
                        .catch(() => {});
    }
    else
    {
        graph.drawing.working_data
            .attr("x", function(d) { return node_x(d); })
            .attr("y", function(d) { return node_y(d); })
            .attr("width", function(d) { return node_width(d); })
            .attr("height", function(d) { return node_height(d); });
        render_nodes(render_canvas.node());
    }
    reset_view();
}

function navigate_to_decl_if_possible(index) {
    // Node names correspond to the stringified handle value.
    //const found = graph.drawing.working_data.nodes().find(e => e.__data__.data.name == matching_name);
    const found = graph.drawing.working_data.nodes().find(e => {
        const name = e.__data__.data.key;
        if (!is_meta_name(name))
            return false;
        const meta = name_and_index_from_meta(name);
        return meta.index.sort == index.sort && meta.index.index == index.index;
    });
    if (found == undefined)
        return;
    navigate_to(found.__data__, SkipSelection);
}