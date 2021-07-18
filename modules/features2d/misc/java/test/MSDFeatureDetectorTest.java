package org.opencv.test.features2d;

import org.opencv.test.OpenCVTestCase;
import org.opencv.test.OpenCVTestRunner;
import org.opencv.xfeatures2d.MSDDetector;

public class MSDFeatureDetectorTest extends OpenCVTestCase {

    MSDDetector detector;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        detector = MSDDetector.create(); // default (3,5,5,0,250.4,',1.25,-1,false)
    }

    public void testCreate() {
        assertNotNull(detector);
    }

    public void testDetectListOfMatListOfListOfKeyPoint() {
        fail("Not yet implemented");
    }

    public void testDetectListOfMatListOfListOfKeyPointListOfMat() {
        fail("Not yet implemented");
    }

    public void testDetectMatListOfKeyPoint() {
        fail("Not yet implemented");
    }

    public void testDetectMatListOfKeyPointMat() {
        fail("Not yet implemented");
    }

    public void testEmpty() {
        fail("Not yet implemented");
    }

    public void testRead() {
        String filename = OpenCVTestRunner.getTempFileName("xml");
        writeFile(filename, "<?xml version=\"1.0\"?>\n<opencv_storage>\n<name>Feature2D.MSD</name>\n<patch_radius>4</patch_radius>\n<search_area_radius>6</search_area_radius>\n<nms_radius>7</nms_radius>\n<nms_scale_radius>1</nms_scale_radius>\n<th_saliency>251.</th_saliency>\n<kNN>2</kNN>\n<scale_factor>1.26</scale_factor>\n<n_scales>3</n_scales>\n<compute_orientation>1</compute_orientation>\n</opencv_storage>\n");
        detector.read(filename);

        assertEquals(4, detector.getPatchRadius());
        assertEquals(6, detector.getSearchAreaRadius());
        assertEquals(7, detector.getNmsRadius());
        assertEquals(1, detector.getNmsScaleRadius());
        assertEquals((float) 251.0, detector.getThSaliency());
        assertEquals(2, detector.getKNN());
        assertEquals((float) 1.26, detector.getScaleFactor());
        assertEquals(3, detector.getNScales());
        assertEquals(true, detector.getComputeOrientation());
    }

    public void testReadYml() {
        String filename = OpenCVTestRunner.getTempFileName("yml");
        writeFile(filename, "%YAML:1.0\n---\nname: \"Feature2D.MSD\"\npatch_radius: 4\nsearch_area_radius: 6\nnms_radius: 7\nnms_scale_radius: 1\nth_saliency: 251.\nkNN: 2\nscale_factor: 1.26\nn_scales: 3\ncompute_orientation: 1\n");

        detector.read(filename);

        assertEquals(4, detector.getPatchRadius());
        assertEquals(6, detector.getSearchAreaRadius());
        assertEquals(7, detector.getNmsRadius());
        assertEquals(1, detector.getNmsScaleRadius());
        assertEquals((float) 251.0, detector.getThSaliency());
        assertEquals(2, detector.getKNN());
        assertEquals((float) 1.26, detector.getScaleFactor());
        assertEquals(3, detector.getNScales());
        assertEquals(true, detector.getComputeOrientation());
    }

    public void testWrite() {
        String filename = OpenCVTestRunner.getTempFileName("xml");

        detector.write(filename);

        String truth = "<?xml version=\"1.0\"?>\n<opencv_storage>\n<name>Feature2D.MSD</name>\n<patch_radius>3</patch_radius>\n<search_area_radius>5</search_area_radius>\n<nms_radius>5</nms_radius>\n<nms_scale_radius>0</nms_scale_radius>\n<th_saliency>250.</th_saliency>\n<kNN>4</kNN>\n<scale_factor>1.2500000000000000e+00</scale_factor>\n<n_scales>-1</n_scales>\n<compute_orientation>0</compute_orientation>\n</opencv_storage>\n";
        String actual = readFile(filename);
        actual = actual.replaceAll("e([+-])0(\\d\\d)", "e$1$2"); // NOTE: workaround for different platforms double representation
        assertEquals(truth, actual);
    }

    public void testWriteYml() {
        String filename = OpenCVTestRunner.getTempFileName("yml");

        detector.write(filename);

        String truth = "%YAML:1.0\n---\nname: \"Feature2D.MSD\"\npatch_radius: 3\nsearch_area_radius: 5\nnms_radius: 5\nnms_scale_radius: 0\nth_saliency: 250.\nkNN: 4\nscale_factor: 1.2500000000000000e+00\nn_scales: -1\ncompute_orientation: 0\n";
        String actual = readFile(filename);
        actual = actual.replaceAll("e([+-])0(\\d\\d)", "e$1$2"); // NOTE: workaround for different platforms double representation
        assertEquals(truth, actual);
    }

}
